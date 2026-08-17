/**
 * @file DuckCrypto.h
 * @brief ChaCha20-Poly1305 (IETF) authenticated encryption wrapper built on
 * top of this Duck's DuckIdentity X25519 keypair. Implements the two
 * encryption modes described in docs/crypto-design.tex:
 *
 *  - Session mode (encryptWithPeer/decryptFromPeer): used for Duck<->Duck
 *    mesh traffic and for decrypting OpenDMS's SOS-ack replies. Derives a
 *    shared key via static-static X25519 ECDH between THIS Duck's own
 *    long-term identity private key and a peer's long-term public key.
 *    ECDH is symmetric, so the same function correctly decrypts anything
 *    encrypted against this Duck's public key by a peer using its own
 *    long-term private key -- including OpenDMS's static keypair.
 *
 *  - Sealed/ephemeral mode (sealToStatic): used for one-way Duck->OpenDMS
 *    uplink traffic. Generates a fresh, one-time X25519 keypair per call
 *    and derives a shared key via ECDH against a fixed, pinned destination
 *    static public key. The caller must transmit the returned ephemeral
 *    public key alongside the ciphertext, since it is the only way the
 *    destination can derive the same shared key.
 *
 *  - Group mode (encryptWithGroupKey/decryptWithGroupKey): used for
 *    local mesh discovery broadcasts (e.g. BEACON/BEACON_ACK) meant to be
 *    readable by any Duck in the same deployment, not just one known peer
 *    (session mode) or OpenDMS (sealed mode). Uses a pre-shared symmetric
 *    key directly as the ChaCha20-Poly1305 key -- no ECDH involved, so
 *    there is no per-recipient key derivation and any holder of the group
 *    key can both encrypt and decrypt. See src/security/MeshGroupConfig.h
 *    for how the group key is provisioned.
 *
 * All functions require duckidentity::begin() to have already returned
 * DUCK_ERR_NONE.
 *
 * @version
 * @date 2026-08-14
 *
 * @copyright
 */

#ifndef DUCKCRYPTO_H_
#define DUCKCRYPTO_H_

#include <stdint.h>
#include <cstddef>

namespace duckcrypto {

/// X25519 public key size, in bytes.
constexpr size_t PUBLIC_KEY_LENGTH = 32;
/// ChaCha20-Poly1305 key size, in bytes.
constexpr size_t KEY_LENGTH = 32;
/// ChaCha20-Poly1305 (IETF) nonce size, in bytes.
constexpr size_t NONCE_LENGTH = 12;
/// Poly1305 authentication tag size, in bytes.
constexpr size_t TAG_LENGTH = 16;

/**
 * @brief Encrypt a message for a known peer (session mode).
 *
 * Derives a shared key via X25519 ECDH between this Duck's own identity
 * private key and `peerPublicKey`, then encrypts `plaintext` with
 * ChaCha20-Poly1305. A fresh random nonce is generated for this call and
 * returned via `outNonce`; the caller must transmit it alongside the
 * ciphertext and tag, since the destination needs it to decrypt.
 *
 * @param peerPublicKey PUBLIC_KEY_LENGTH-byte long-term public key of the
 * intended recipient.
 * @param aad optional additional authenticated data (e.g. the CDP header),
 * authenticated but not encrypted. May be nullptr if aadLen is 0.
 * @param aadLen length of aad, in bytes.
 * @param plaintext data to encrypt.
 * @param plaintextLen length of plaintext, in bytes.
 * @param outNonce output buffer of at least NONCE_LENGTH bytes.
 * @param outCiphertext output buffer of at least plaintextLen bytes.
 * @param outTag output buffer of at least TAG_LENGTH bytes.
 * @returns DUCK_ERR_NONE on success, or a DUCK_ERR_CRYPTO_* error code.
 */
int encryptWithPeer(const uint8_t* peerPublicKey,
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* plaintext, size_t plaintextLen,
                     uint8_t* outNonce,
                     uint8_t* outCiphertext,
                     uint8_t* outTag);

/**
 * @brief Decrypt a message from a known peer (session mode).
 *
 * Mirrors encryptWithPeer(). Also correctly decrypts messages encrypted
 * by OpenDMS's SOS-ack reply path, since ECDH is symmetric: pass OpenDMS's
 * static public key as `peerPublicKey`.
 *
 * @param peerPublicKey PUBLIC_KEY_LENGTH-byte long-term public key of the
 * sender.
 * @param nonce NONCE_LENGTH-byte nonce, as received alongside the message.
 * @param aad optional additional authenticated data, must match what the
 * sender authenticated. May be nullptr if aadLen is 0.
 * @param aadLen length of aad, in bytes.
 * @param ciphertext encrypted data, as received.
 * @param ciphertextLen length of ciphertext, in bytes.
 * @param tag TAG_LENGTH-byte authentication tag, as received.
 * @param outPlaintext output buffer of at least ciphertextLen bytes. Left
 * zeroed if authentication fails.
 * @returns DUCK_ERR_NONE on success, or DUCK_ERR_CRYPTO_AUTH_FAILED if the
 * tag does not match (message must be discarded), or another
 * DUCK_ERR_CRYPTO_* error code.
 */
int decryptFromPeer(const uint8_t* peerPublicKey,
                     const uint8_t* nonce,
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* ciphertext, size_t ciphertextLen,
                     const uint8_t* tag,
                     uint8_t* outPlaintext);

/**
 * @brief Seal a message for a fixed, pinned static destination public key
 * (sealed/ephemeral mode), e.g. Duck->OpenDMS uplink traffic.
 *
 * Generates a fresh, one-time X25519 keypair for this call only, derives
 * a shared key via ECDH against `destStaticPublicKey`, and encrypts
 * `plaintext` with ChaCha20-Poly1305. The caller must transmit
 * `outEphemeralPublicKey`, `outNonce`, the ciphertext and the tag together,
 * since the ephemeral public key is the only way the destination can
 * derive the same shared key.
 *
 * @param destStaticPublicKey PUBLIC_KEY_LENGTH-byte fixed public key of
 * the destination (e.g. OpenDMS's pinned static public key).
 * @param aad optional additional authenticated data. May be nullptr if
 * aadLen is 0.
 * @param aadLen length of aad, in bytes.
 * @param plaintext data to encrypt.
 * @param plaintextLen length of plaintext, in bytes.
 * @param outEphemeralPublicKey output buffer of at least
 * PUBLIC_KEY_LENGTH bytes.
 * @param outNonce output buffer of at least NONCE_LENGTH bytes.
 * @param outCiphertext output buffer of at least plaintextLen bytes.
 * @param outTag output buffer of at least TAG_LENGTH bytes.
 * @returns DUCK_ERR_NONE on success, or a DUCK_ERR_CRYPTO_* error code.
 */
int sealToStatic(const uint8_t* destStaticPublicKey,
                  const uint8_t* aad, size_t aadLen,
                  const uint8_t* plaintext, size_t plaintextLen,
                  uint8_t* outEphemeralPublicKey,
                  uint8_t* outNonce,
                  uint8_t* outCiphertext,
                  uint8_t* outTag);

/**
 * @brief Encrypt a message using a pre-shared symmetric group key (group
 * mode).
 *
 * Unlike encryptWithPeer()/sealToStatic(), this performs no X25519 ECDH --
 * `groupKey` is used directly as the ChaCha20-Poly1305 key, so any device
 * holding the same group key can decrypt it. Intended for broadcast
 * discovery traffic meant to be readable by any Duck in the deployment
 * (e.g. BEACON/BEACON_ACK), where session mode (single known peer) and
 * sealed mode (OpenDMS only) don't fit.
 *
 * @param groupKey KEY_LENGTH-byte pre-shared symmetric key (see
 * src/security/MeshGroupConfig.h).
 * @param aad optional additional authenticated data, authenticated but
 * not encrypted. May be nullptr if aadLen is 0.
 * @param aadLen length of aad, in bytes.
 * @param plaintext data to encrypt.
 * @param plaintextLen length of plaintext, in bytes.
 * @param outNonce output buffer of at least NONCE_LENGTH bytes.
 * @param outCiphertext output buffer of at least plaintextLen bytes.
 * @param outTag output buffer of at least TAG_LENGTH bytes.
 * @returns DUCK_ERR_NONE on success.
 */
int encryptWithGroupKey(const uint8_t* groupKey,
                         const uint8_t* aad, size_t aadLen,
                         const uint8_t* plaintext, size_t plaintextLen,
                         uint8_t* outNonce,
                         uint8_t* outCiphertext,
                         uint8_t* outTag);

/**
 * @brief Decrypt a message using a pre-shared symmetric group key (group
 * mode). Mirrors encryptWithGroupKey().
 *
 * @param groupKey KEY_LENGTH-byte pre-shared symmetric key.
 * @param nonce NONCE_LENGTH-byte nonce, as received alongside the message.
 * @param aad optional additional authenticated data, must match what the
 * sender authenticated. May be nullptr if aadLen is 0.
 * @param aadLen length of aad, in bytes.
 * @param ciphertext encrypted data, as received.
 * @param ciphertextLen length of ciphertext, in bytes.
 * @param tag TAG_LENGTH-byte authentication tag, as received.
 * @param outPlaintext output buffer of at least ciphertextLen bytes. Left
 * zeroed if authentication fails.
 * @returns DUCK_ERR_NONE on success, or DUCK_ERR_CRYPTO_AUTH_FAILED if the
 * tag does not match (message must be discarded).
 */
int decryptWithGroupKey(const uint8_t* groupKey,
                        const uint8_t* nonce,
                        const uint8_t* aad, size_t aadLen,
                        const uint8_t* ciphertext, size_t ciphertextLen,
                        const uint8_t* tag,
                        uint8_t* outPlaintext);

} // namespace duckcrypto

#endif
