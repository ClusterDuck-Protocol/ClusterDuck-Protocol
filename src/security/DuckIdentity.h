/**
 * @file DuckIdentity.h
 * @brief This file is internal to CDP and provides this Duck's self-sovereign
 * cryptographic identity: a randomly generated X25519 keypair that is
 * persisted to flash on first boot and never requires the user to back up
 * or memorize anything. See docs/crypto-design.tex for the full design
 * rationale.
 *
 * @version
 * @date 2026-08-14
 *
 * @copyright
 */

#ifndef DUCKIDENTITY_H_
#define DUCKIDENTITY_H_

#include "../CdpPacket.h"
#include <stdint.h>
#include <cstddef>

namespace duckidentity {

/// Curve25519 public key size, in bytes.
constexpr size_t PUBLIC_KEY_LENGTH = 32;
/// Curve25519 private key size, in bytes.
constexpr size_t PRIVATE_KEY_LENGTH = 32;

/**
 * @brief Initialize this Duck's cryptographic identity.
 *
 * On first boot, generates a fresh X25519 keypair using the platform's
 * hardware random number source and persists it to flash. On subsequent
 * boots, loads the previously generated keypair instead. The private key
 * never leaves the device and is never logged, displayed, or transmitted.
 *
 * Must be called once, early in setup, before any code that depends on
 * this Duck's identity or DUID.
 *
 * @returns DUCK_ERR_NONE on success, or a DUCK_ERR_IDENTITY_* error code.
 */
int begin();

/**
 * @brief Get this Duck's public key.
 *
 * @returns pointer to a PUBLIC_KEY_LENGTH-byte buffer. Only valid after
 * begin() has returned DUCK_ERR_NONE.
 */
const uint8_t* getPublicKey();

/**
 * @brief Get this Duck's private key.
 *
 * Internal use only (e.g. by future Duck-to-Duck / Duck-to-OpenDMS
 * encryption code performing ECDH). Never log, display, or transmit
 * this value.
 *
 * @returns pointer to a PRIVATE_KEY_LENGTH-byte buffer. Only valid after
 * begin() has returned DUCK_ERR_NONE.
 */
const uint8_t* getPrivateKey();

/**
 * @brief Derive this Duck's DUID from its public key.
 *
 * The DUID is a truncated hash of the public key, making it
 * self-certifying (Reticulum-style) rather than an arbitrary value.
 *
 * @param duid output buffer of at least DUID_LENGTH bytes.
 */
void getDuid(uint8_t* duid);

/**
 * @brief Erase the stored identity so a new one is generated on next begin().
 *
 * This is the mechanism that replaces "backing up" a private key: if a
 * device's identity is lost (factory reset, flash erase), it simply
 * generates a new one and re-announces on the mesh.
 *
 * @returns DUCK_ERR_NONE on success, or a DUCK_ERR_IDENTITY_* error code.
 */
int reset();

} // namespace duckidentity

#endif
