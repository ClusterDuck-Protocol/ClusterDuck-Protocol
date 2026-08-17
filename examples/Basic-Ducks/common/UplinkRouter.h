/**
 * @file UplinkRouter.h
 * @brief Shared uplink/MTALK routing helpers for MamaDuck-based example
 * sketches (Heltec, Seeed Wio Tracker L1 Pro, and any future board).
 *
 * These functions were originally duplicated, near byte-for-byte, across
 * each example's MamaDuck.ino. They contain no display/BLE/GPS-hardware
 * dependencies -- only calls into the shared Duck/MamaDuck API -- so they
 * are factored out here to avoid re-typing (and re-drifting) the same
 * routing/fail-open/fail-closed policy in every new sketch.
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

#ifndef UPLINKROUTER_H_
#define UPLINKROUTER_H_

#include <array>
#include <set>
#include <string>
#include <Arduino.h>
#include "Ducks/MamaDuck.h"
#include "CdpPacket.h"
#include "utils/DuckError.h"

extern MamaDuck<DuckWifiNone, DuckLoRa> duck;

// Routes GPS/alert/status/roger uplink sends through sendSealedData() (one-
// way seal to OpenDMS's pinned static public key, src/security/OpenDmsConfig.h)
// when the operator has enabled uplink encryption (duck.isUplinkEncryptionEnabled(),
// off by default -- see Duck.h's setUplinkEncryptionEnabled()); falls back to
// plain duck.sendData() otherwise. MamaDuck-to-MamaDuck traffic (MTALK, topic
// 26) is intentionally NOT routed through here -- that's session-mode via
// sendEncryptedData()/announceIdentity(), targeting a peer Duck's identity
// key, not OpenDMS's static key.
inline int sendUplink(uint8_t topic, const std::string data,
                       const std::array<uint8_t, 8> targetDevice = PAPADUCK_DUID) {
    if (duck.isUplinkEncryptionEnabled()) {
        return duck.sendSealedData(topic, data, targetDevice);
    }
    return duck.sendData(topic, data, targetDevice);
}

// SOS-only variant of sendUplink(): tries to seal via OpenDMS first, same as
// every other uplink report, but -- unlike sendUplink() -- falls back to a
// last-resort plaintext duck.sendData() if sealing fails (e.g. OpenDMS not
// yet configured / ECDH failure). For life-safety SOS alerts, the operator
// has explicitly chosen availability over confidentiality: better to leak
// GPS location than to silently drop an emergency alert. Only use this for
// the SOS panic-button flow (sendEmergency()/handleSOS()) -- everything
// else (routine GPS/status reports, handleMsg()) stays fail-closed via
// plain sendUplink() above.
inline int sendUplinkSos(uint8_t topic, const std::string data,
                          const std::array<uint8_t, 8> targetDevice = PAPADUCK_DUID) {
    int rc = sendUplink(topic, data, targetDevice);
    if (rc != DUCK_ERR_NONE && duck.isUplinkEncryptionEnabled()) {
        Serial.println("[MAMA] SOS seal failed -- falling back to cleartext (availability > confidentiality).");
        return duck.sendData(topic, data, targetDevice);
    }
    return rc;
}

// Peers we've directed-announced our own identity to during this boot
// session, so sendMamaLink() only does it once per peer instead of on every
// message. This closes a one-directional key-exchange gap: the one-time
// BROADCAST_DUID announceIdentity() in setup() may never reach a given peer
// (e.g. it boots later, or was out of range at the time), so that peer can
// still send us plaintext MTALK (fine, decodes regardless) but has no way
// to know we can't yet decrypt anything IT encrypts for US -- and we have
// no way to know whether IT already has OUR key either. Concretely: if we
// receive an MTALK message and reply with an encrypted MTALK_ACK receipt via
// sendEncryptedData(), and the original sender never learned our public
// key, tryDecryptEncryptedData() on their end silently drops our ACK (no
// known public key for sender) -- the message text still shows up fine on
// their phone, but the delivery-receipt tick never updates. A directed
// identity_announce() to that specific peer, sent right before we first
// encrypt anything for it, closes that gap.
inline std::set<std::array<uint8_t, 8>> announcedIdentityTo;

// Timestamp of the last periodic (broadcast) identity re-announce -- read
// and written from each sketch's loop().
inline unsigned long lastIdentityAnnounceMs = 0;

// Routes MamaDuck-to-MamaDuck (MTALK, topic 26) sends through
// sendEncryptedData() -- session-mode X25519 ECDH between this Duck's and
// the peer's long-term identities (see duck.announceIdentity() in setup()
// and Duck.h's learnPeerIdentity()). This is intentionally separate from
// sendUplink() above: MTALK is Duck<->Duck session-mode traffic sealed to a
// peer's identity key, NOT OpenDMS's static uplink key.
//
// Fail-closed: MTALK encryption is permanent (Duck::isMamaLinkEncryptionEnabled()
// always true) -- never falls back to plaintext duck.sendData() here, even
// if sendEncryptedData() fails because this peer's identity key isn't known
// yet. The periodic announceIdentity() re-broadcast in loop() closes that
// gap over time.
inline int sendMamaLink(const std::string& data, const std::array<uint8_t, 8>& targetDuid) {
    if (announcedIdentityTo.insert(targetDuid).second) {
        duck.announceIdentity(targetDuid);
    }
    return duck.sendEncryptedData(26, data, targetDuid);
}

#endif  // UPLINKROUTER_H_
