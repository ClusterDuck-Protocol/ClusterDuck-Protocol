/**
 * @file DuckCrypto.cpp
 * @brief Implementation of the ChaCha20-Poly1305 wrapper described in
 * DuckCrypto.h. See DuckCrypto.h and docs/crypto-design.tex for the design
 * rationale.
 */

#include "DuckCrypto.h"
#include "DuckIdentity.h"
#include "../utils/DuckError.h"
#include "../utils/DuckLogger.h"

#include <Curve25519.h>
#include <ChaChaPoly.h>
#include <HKDF.h>
#include <SHA256.h>
#include <RNG.h>
#include <string.h>

namespace duckcrypto {

namespace {

const char HKDF_INFO[] = "meshbeacon-firmware DuckCrypto";

// Performs X25519 ECDH between `privateKey` and `peerPublicKey`, then runs
// the raw 32-byte shared secret through HKDF-SHA256 to derive a
// ChaChaPoly key.
//
// IMPORTANT: Curve25519::dh2() both reads AND destroys `privateKey`
// (clears it to zero internally once used), so callers must always pass a
// throwaway copy of a private key here, never DuckIdentity's live buffer
// directly.
int deriveKey(const uint8_t* peerPublicKey, uint8_t* privateKey, uint8_t* outKey) {
  uint8_t shared[32];
  memcpy(shared, peerPublicKey, sizeof(shared));
  if (!Curve25519::dh2(shared, privateKey)) {
    memset(shared, 0, sizeof(shared));
    logerr_ln("DuckCrypto: ECDH exchange rejected peer public key");
    return DUCK_ERR_CRYPTO_ECDH_FAILED;
  }
  hkdf<SHA256>(outKey, KEY_LENGTH, shared, sizeof(shared), nullptr, 0,
               HKDF_INFO, sizeof(HKDF_INFO) - 1);
  memset(shared, 0, sizeof(shared));
  return DUCK_ERR_NONE;
}

} // namespace

int encryptWithPeer(const uint8_t* peerPublicKey,
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* plaintext, size_t plaintextLen,
                     uint8_t* outNonce,
                     uint8_t* outCiphertext,
                     uint8_t* outTag) {
  uint8_t privateKeyCopy[duckidentity::PRIVATE_KEY_LENGTH];
  memcpy(privateKeyCopy, duckidentity::getPrivateKey(), sizeof(privateKeyCopy));

  uint8_t key[KEY_LENGTH];
  int rc = deriveKey(peerPublicKey, privateKeyCopy, key);
  memset(privateKeyCopy, 0, sizeof(privateKeyCopy));
  if (rc != DUCK_ERR_NONE) {
    return rc;
  }

  CryptRNG.rand(outNonce, NONCE_LENGTH);

  ChaChaPoly cipher;
  cipher.setKey(key, KEY_LENGTH);
  cipher.setIV(outNonce, NONCE_LENGTH);
  if (aadLen > 0) {
    cipher.addAuthData(aad, aadLen);
  }
  cipher.encrypt(outCiphertext, plaintext, plaintextLen);
  cipher.computeTag(outTag, TAG_LENGTH);
  cipher.clear();
  memset(key, 0, sizeof(key));
  return DUCK_ERR_NONE;
}

int decryptFromPeer(const uint8_t* peerPublicKey,
                     const uint8_t* nonce,
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* ciphertext, size_t ciphertextLen,
                     const uint8_t* tag,
                     uint8_t* outPlaintext) {
  uint8_t privateKeyCopy[duckidentity::PRIVATE_KEY_LENGTH];
  memcpy(privateKeyCopy, duckidentity::getPrivateKey(), sizeof(privateKeyCopy));

  uint8_t key[KEY_LENGTH];
  int rc = deriveKey(peerPublicKey, privateKeyCopy, key);
  memset(privateKeyCopy, 0, sizeof(privateKeyCopy));
  if (rc != DUCK_ERR_NONE) {
    return rc;
  }

  ChaChaPoly cipher;
  cipher.setKey(key, KEY_LENGTH);
  cipher.setIV(nonce, NONCE_LENGTH);
  if (aadLen > 0) {
    cipher.addAuthData(aad, aadLen);
  }
  cipher.decrypt(outPlaintext, ciphertext, ciphertextLen);
  bool ok = cipher.checkTag(tag, TAG_LENGTH);
  cipher.clear();
  memset(key, 0, sizeof(key));

  if (!ok) {
    memset(outPlaintext, 0, ciphertextLen);
    logerr_ln("DuckCrypto: decryptFromPeer authentication failed, discarding message");
    return DUCK_ERR_CRYPTO_AUTH_FAILED;
  }
  return DUCK_ERR_NONE;
}

int sealToStatic(const uint8_t* destStaticPublicKey,
                  const uint8_t* aad, size_t aadLen,
                  const uint8_t* plaintext, size_t plaintextLen,
                  uint8_t* outEphemeralPublicKey,
                  uint8_t* outNonce,
                  uint8_t* outCiphertext,
                  uint8_t* outTag) {
  uint8_t ephemeralPrivateKey[duckidentity::PRIVATE_KEY_LENGTH];
  // Generates a fresh one-time keypair using CryptRNG: outEphemeralPublicKey
  // is the public half, ephemeralPrivateKey the private half. The private
  // half is used exactly once below and is destroyed by dh2() inside
  // deriveKey(), matching Curve25519::dh1()/dh2()'s intended one-shot usage.
  Curve25519::dh1(outEphemeralPublicKey, ephemeralPrivateKey);

  uint8_t key[KEY_LENGTH];
  int rc = deriveKey(destStaticPublicKey, ephemeralPrivateKey, key);
  memset(ephemeralPrivateKey, 0, sizeof(ephemeralPrivateKey));
  if (rc != DUCK_ERR_NONE) {
    return rc;
  }

  CryptRNG.rand(outNonce, NONCE_LENGTH);

  ChaChaPoly cipher;
  cipher.setKey(key, KEY_LENGTH);
  cipher.setIV(outNonce, NONCE_LENGTH);
  if (aadLen > 0) {
    cipher.addAuthData(aad, aadLen);
  }
  cipher.encrypt(outCiphertext, plaintext, plaintextLen);
  cipher.computeTag(outTag, TAG_LENGTH);
  cipher.clear();
  memset(key, 0, sizeof(key));
  return DUCK_ERR_NONE;
}

int encryptWithGroupKey(const uint8_t* groupKey,
                         const uint8_t* aad, size_t aadLen,
                         const uint8_t* plaintext, size_t plaintextLen,
                         uint8_t* outNonce,
                         uint8_t* outCiphertext,
                         uint8_t* outTag) {
  CryptRNG.rand(outNonce, NONCE_LENGTH);

  ChaChaPoly cipher;
  cipher.setKey(groupKey, KEY_LENGTH);
  cipher.setIV(outNonce, NONCE_LENGTH);
  if (aadLen > 0) {
    cipher.addAuthData(aad, aadLen);
  }
  cipher.encrypt(outCiphertext, plaintext, plaintextLen);
  cipher.computeTag(outTag, TAG_LENGTH);
  cipher.clear();
  return DUCK_ERR_NONE;
}

int decryptWithGroupKey(const uint8_t* groupKey,
                        const uint8_t* nonce,
                        const uint8_t* aad, size_t aadLen,
                        const uint8_t* ciphertext, size_t ciphertextLen,
                        const uint8_t* tag,
                        uint8_t* outPlaintext) {
  ChaChaPoly cipher;
  cipher.setKey(groupKey, KEY_LENGTH);
  cipher.setIV(nonce, NONCE_LENGTH);
  if (aadLen > 0) {
    cipher.addAuthData(aad, aadLen);
  }
  cipher.decrypt(outPlaintext, ciphertext, ciphertextLen);
  bool ok = cipher.checkTag(tag, TAG_LENGTH);
  cipher.clear();

  if (!ok) {
    memset(outPlaintext, 0, ciphertextLen);
    logerr_ln("DuckCrypto: decryptWithGroupKey authentication failed, discarding message");
    return DUCK_ERR_CRYPTO_AUTH_FAILED;
  }
  return DUCK_ERR_NONE;
}

} // namespace duckcrypto
