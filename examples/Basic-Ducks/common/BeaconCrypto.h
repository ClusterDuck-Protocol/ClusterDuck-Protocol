/**
 * @file BeaconCrypto.h
 * @brief Shared BEACON group-key encryption and Emergency Broadcast
 * authentication helpers for MamaDuck-based example sketches (Heltec,
 * Seeed Wio Tracker L1 Pro, and any future board).
 *
 * These functions were originally duplicated, byte-for-byte, across each
 * example's MamaDuck.ino. They contain no display/BLE/GPS-hardware
 * dependencies -- only calls into the shared Duck/DuckCrypto/MeshGroupConfig
 * API -- so they are factored out here to avoid re-typing (and re-drifting)
 * the same crypto/authentication logic in every new sketch.
 *
 * Lives under examples/Basic-Ducks/common/ (not src/) because it is
 * example-sketch glue code, not part of the CDP library itself -- keeping
 * it out of src/ avoids mixing sketch-level conventions (e.g. requiring a
 * global `duck` instance) into the upstream library's own directory
 * structure.
 *
 * Requires the including .ino to have already declared a global MamaDuck
 * instance named exactly `duck` (e.g. `MamaDuck duck(DUCK_NAME);`), as is
 * the convention in every existing example.
 */

#ifndef BEACONCRYPTO_H_
#define BEACONCRYPTO_H_

#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include "Ducks/MamaDuck.h"
#include "CdpPacket.h"
#include "security/DuckCrypto.h"
#include "security/MeshGroupConfig.h"
#include "utils/DuckError.h"

extern MamaDuck<DuckWifiNone, DuckLoRa> duck;

// ── BEACON group-key encryption ─────────────────────────────────────────
// TOPIC_BEACON/TOPIC_BEACON_ACK broadcast GPS coordinates for local
// discovery, so neither of Duck.h's existing encryption modes fit:
// sendSealedData() is only readable by OpenDMS, sendEncryptedData() only
// by one already-known peer. If a mesh group key is provisioned (see
// src/security/MeshGroupConfig.h), wrap the payload with
// duckcrypto::encryptWithGroupKey() so it's opaque to eavesdroppers
// outside this deployment while still readable by any duck holding the
// same key. Falls back to sending in the clear if no group key is
// configured, so discovery keeps working before/without provisioning.
//
// Replay protection: a 4-byte big-endian monotonic counter (incremented
// on this device every time it sends a BEACON or BEACON_ACK) is bound
// into the AAD, and receivers reject anything not strictly greater than
// the last counter seen from that specific sender -- otherwise a captured
// packet (``duck X at location Y'') could be replayed later to make a
// stale cached location look current in duckGpsCache. Unlike Emergency
// Broadcast's single authority (OpenDMS), every Duck sends its own
// BEACON/BEACON_ACK, so freshness is tracked per-sender (keyed by the raw
// 8-byte DUID) rather than with one global baseline. Like Emergency
// Broadcast's counter, this baseline is RAM-only (not persisted to
// flash), so a reboot resets a sender's baseline to 0 and a captured
// packet from that sender could be replayed once, immediately after a
// reboot, before a fresh one arrives from them -- the same accepted
// residual gap documented for Emergency Broadcast, see
// docs/crypto-design.tex.
inline const uint8_t BEACON_GROUP_MARKER = 0xE6;
inline const size_t BEACON_COUNTER_LENGTH = 4;
inline uint32_t nextBeaconCounter = 0;
inline std::unordered_map<std::string, uint32_t> lastSeenBeaconCounter;

inline std::string encryptBeaconPayload(uint8_t topic, const char* plaintext) {
  size_t plaintextLen = strlen(plaintext);
  uint32_t counter = ++nextBeaconCounter;
  uint8_t counterBytes[BEACON_COUNTER_LENGTH];
  counterBytes[0] = uint8_t(counter >> 24);
  counterBytes[1] = uint8_t(counter >> 16);
  counterBytes[2] = uint8_t(counter >> 8);
  counterBytes[3] = uint8_t(counter);

  // AAD = topic(1) || sduid(8) || counter(4, big-endian) -- the counter
  // must be authenticated (tamper-proof) even though it also appears in
  // the clear in the wire framing below, so a receiver can trust it
  // before deciding whether the packet is fresh.
  uint8_t aad[9 + BEACON_COUNTER_LENGTH];
  aad[0] = topic;
  memcpy(aad + 1, duck.getDuckId().data(), 8);
  memcpy(aad + 9, counterBytes, BEACON_COUNTER_LENGTH);

  std::vector<uint8_t> wire(1 + duckcrypto::NONCE_LENGTH + BEACON_COUNTER_LENGTH
                             + plaintextLen + duckcrypto::TAG_LENGTH);
  wire[0] = BEACON_GROUP_MARKER;
  uint8_t* nonce        = wire.data() + 1;
  uint8_t* counterField = nonce + duckcrypto::NONCE_LENGTH;
  uint8_t* ciphertext   = counterField + BEACON_COUNTER_LENGTH;
  uint8_t* tag          = ciphertext + plaintextLen;
  memcpy(counterField, counterBytes, BEACON_COUNTER_LENGTH);
  duckcrypto::encryptWithGroupKey(meshgroupconfig::MESH_GROUP_KEY, aad, sizeof(aad),
                                  (const uint8_t*)plaintext, plaintextLen,
                                  nonce, ciphertext, tag);
  return std::string(reinterpret_cast<const char*>(wire.data()), wire.size());
}

// Returns true and fills outPlaintext if `data` is a group-key-encrypted,
// not-already-seen BEACON payload that was successfully decrypted.
// Returns false (leaves outPlaintext untouched) if it isn't marked as
// encrypted, no group key is configured, authentication fails, or the
// counter is not newer than the last one accepted from this sender
// (replay) -- callers should treat that the same as any other
// unrecognized/plaintext payload.
inline bool decryptBeaconPayload(uint8_t topic, const uint8_t* sduid,
                                  const std::vector<uint8_t>& data,
                                  std::string& outPlaintext) {
  if (data.size() < 1 + duckcrypto::NONCE_LENGTH + BEACON_COUNTER_LENGTH + duckcrypto::TAG_LENGTH
      || data[0] != BEACON_GROUP_MARKER || !meshgroupconfig::isConfigured()) {
    return false;
  }
  const uint8_t* nonce = data.data() + 1;
  const uint8_t* counterBytes = nonce + duckcrypto::NONCE_LENGTH;
  const uint8_t* ciphertext = counterBytes + BEACON_COUNTER_LENGTH;
  size_t ciphertextLen = data.size() - 1 - duckcrypto::NONCE_LENGTH - BEACON_COUNTER_LENGTH
                         - duckcrypto::TAG_LENGTH;
  const uint8_t* tag = ciphertext + ciphertextLen;

  uint8_t aad[9 + BEACON_COUNTER_LENGTH];
  aad[0] = topic;
  memcpy(aad + 1, sduid, 8);
  memcpy(aad + 9, counterBytes, BEACON_COUNTER_LENGTH);

  std::vector<uint8_t> plain(ciphertextLen);
  int rc = duckcrypto::decryptWithGroupKey(meshgroupconfig::MESH_GROUP_KEY, nonce,
                                           aad, sizeof(aad), ciphertext, ciphertextLen,
                                           tag, plain.data());
  if (rc != DUCK_ERR_NONE) {
    return false;
  }

  // Counter is only trustworthy once the tag above has verified -- an
  // attacker can't forge a higher counter without the group key, but we
  // must not check freshness before authenticity. Tracked per sender
  // (keyed by the raw 8-byte DUID, NOT duckutils::toString() -- see
  // Neighbor.h -- so non-printable DUID bytes don't collapse distinct
  // senders into the same bucket).
  uint32_t counter = (uint32_t(counterBytes[0]) << 24) | (uint32_t(counterBytes[1]) << 16)
                   | (uint32_t(counterBytes[2]) << 8) | uint32_t(counterBytes[3]);
  std::string senderKey(reinterpret_cast<const char*>(sduid), 8);
  auto seen = lastSeenBeaconCounter.find(senderKey);
  if (seen != lastSeenBeaconCounter.end() && counter <= seen->second) {
    return false;
  }
  lastSeenBeaconCounter[senderKey] = counter;

  outPlaintext.assign(reinterpret_cast<const char*>(plain.data()), plain.size());
  return true;
}


// ── Emergency Broadcast (topic 24) group-key authentication ──────────────────
// Authenticates OpenDMS-originated Emergency Broadcasts with the same
// pre-shared mesh group key as BEACON, since encrypted_cmd can't address a
// broadcast (it's point-to-point: a different shared secret per Duck via
// static-static ECDH). Deliberately authenticated-but-NOT-encrypted: the
// message text travels on-air as cleartext (so anyone in range -- including
// devices that haven't been provisioned with the group key -- can still
// read a life-safety alert), but the mesh group key still prevents anyone
// without it from forging one. Done by calling
// duckcrypto::encryptWithGroupKey()/decryptWithGroupKey() with the message
// bytes passed as AAD (authenticated, not encrypted) and a zero-length
// plaintext -- a standard, secure way to get a MAC (not confidentiality)
// out of an AEAD primitive. AAD is bound to the fixed PAPADUCK_DUID
// placeholder (not this device's own DUID, and not the sender's) since
// OpenDMS has no DUID of its own and cannot predict which physical
// gateway will relay the message -- same convention already used for
// encrypted_cmd's AAD. Must match
// DuckCryptoService::authenticateGroupBroadcast() on the Laravel side
// exactly, byte for byte.
//
// Replay protection: a 4-byte big-endian monotonic counter (incremented
// server-side on every broadcast OpenDMS sends, see
// DuckCryptoService::authenticateGroupBroadcast()) is bound into the AAD
// alongside the message, and this Duck rejects any validly-tagged
// broadcast whose counter is not strictly greater than the last one it
// accepted -- otherwise a captured broadcast (e.g. a stale "all clear")
// could be replayed verbatim to confuse responders even without the
// group key. The last-seen counter is tracked in RAM only (not persisted
// to flash), so a reboot resets the baseline to 0 and a captured
// broadcast could be replayed once, immediately after a reboot, before
// any fresh broadcast arrives -- an accepted residual gap, see
// docs/crypto-design.tex.
// See docs/end-to-end-encryption-setup.md.
inline const uint8_t BROADCAST_AUTH_MARKER = 0xE8;
inline const size_t BROADCAST_COUNTER_LENGTH = 4;
inline uint32_t lastSeenBroadcastCounter = 0;

// Returns true and fills outMessage if `data` is a validly-tagged,
// not-already-seen Emergency Broadcast (message travels as cleartext;
// only the tag is verified). Returns false (leaves outMessage untouched)
// if it isn't marked as authenticated, no group key is configured,
// verification fails, or the counter is not newer than the last accepted
// broadcast (replay) -- callers must treat that the same as any other
// unrecognized/forged payload, NOT as "fall back to trusting the raw
// bytes" once meshgroupconfig::isConfigured().
inline bool verifyBroadcastMac(uint8_t topic, const std::vector<uint8_t>& data,
                                std::string& outMessage) {
  if (data.size() < 1 + duckcrypto::NONCE_LENGTH + BROADCAST_COUNTER_LENGTH + duckcrypto::TAG_LENGTH
      || data[0] != BROADCAST_AUTH_MARKER || !meshgroupconfig::isConfigured()) {
    return false;
  }
  const uint8_t* nonce = data.data() + 1;
  const uint8_t* counterBytes = nonce + duckcrypto::NONCE_LENGTH;
  const uint8_t* message = counterBytes + BROADCAST_COUNTER_LENGTH;
  size_t messageLen = data.size() - 1 - duckcrypto::NONCE_LENGTH - BROADCAST_COUNTER_LENGTH - duckcrypto::TAG_LENGTH;
  const uint8_t* tag = message + messageLen;

  // AAD = topic(1) || PAPADUCK_DUID(8) || counter(4, big-endian) ||
  // message(messageLen) -- must match
  // DuckCryptoService::authenticateGroupBroadcast() exactly, byte for byte.
  std::vector<uint8_t> aad(9 + BROADCAST_COUNTER_LENGTH + messageLen);
  aad[0] = topic;
  memcpy(aad.data() + 1, PAPADUCK_DUID.data(), 8);
  memcpy(aad.data() + 9, counterBytes, BROADCAST_COUNTER_LENGTH);
  memcpy(aad.data() + 9 + BROADCAST_COUNTER_LENGTH, message, messageLen);

  uint8_t unused;
  int rc = duckcrypto::decryptWithGroupKey(meshgroupconfig::MESH_GROUP_KEY, nonce,
                                           aad.data(), aad.size(), &unused, 0,
                                           tag, &unused);
  if (rc != DUCK_ERR_NONE) {
    return false;
  }

  // Counter is only trustworthy once the tag above has verified -- an
  // attacker can't forge a higher counter without the group key, but we
  // must not check freshness before authenticity.
  uint32_t counter = (uint32_t(counterBytes[0]) << 24) | (uint32_t(counterBytes[1]) << 16)
                   | (uint32_t(counterBytes[2]) << 8) | uint32_t(counterBytes[3]);
  if (counter <= lastSeenBroadcastCounter) {
    return false;
  }
  lastSeenBroadcastCounter = counter;

  outMessage.assign(reinterpret_cast<const char*>(message), messageLen);
  return true;
}

#endif  // BEACONCRYPTO_H_
