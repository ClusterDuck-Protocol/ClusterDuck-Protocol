/**
 * @file OpenDmsConfig.h
 * @brief Pins this deployment's OpenDMS instance's fixed, static X25519
 * public key, used to decrypt operator-initiated downlink commands (see
 * docs/crypto-design.tex, "OpenDMS -> Duck (operator-initiated downlink)").
 *
 * OPENDMS_STATIC_PUBLIC_KEY ships as an all-zero placeholder, which
 * isConfigured() reports as "not configured". It can be set three ways:
 *  - build flag: pass -DOPENDMS_STATIC_PUBLIC_KEY_HEX="<64 hex chars>" and
 *    begin() decodes it into OPENDMS_STATIC_PUBLIC_KEY at startup -- see
 *    tools/pubkey_to_c_array.py to convert the meshbeacon Laravel repo's
 *    config/services.php `duck_crypto.public_key` / DUCK_CRYPTO_PUBLIC_KEY
 *    (base64) to hex. NOTE: the flag is named *_HEX, not
 *    OPENDMS_STATIC_PUBLIC_KEY, since the latter is this array's own name
 *    and would break compilation if redefined as a macro.
 *  - compile-time: replace the initializer in OpenDmsConfig.cpp with the
 *    real deployment's key (base64-decoded to raw bytes) before flashing
 *    -- see the meshbeacon Laravel repo's config/services.php
 *    `duck_crypto.public_key` / DUCK_CRYPTO_PUBLIC_KEY.
 *  - field provisioning: begin() loads a previously field-provisioned key
 *    from flash if one was written via the serial provisioning command
 *    (see checkSerialProvisioning()), so a device does not need to be
 *    reflashed just to join a particular OpenDMS instance's mesh.
 *    Overrides the build-flag/compile-time value if present.
 * This key is not secret (only the matching private key, held by OpenDMS,
 * must stay confidential), so compiling it into firmware/source control,
 * persisting it to flash, or writing it over an unauthenticated serial
 * link is fine -- see docs/crypto-design.tex, "Field Operator Onboarding".
 *
 * @version
 * @date 2026-08-14
 *
 * @copyright
 */

#ifndef OPENDMSCONFIG_H_
#define OPENDMSCONFIG_H_

#include <stdint.h>
#include "DuckCrypto.h"

namespace opendmsconfig {

/// This deployment's OpenDMS static X25519 public key. All-zero until
/// begin() loads a field-provisioned key from flash, or the compile-time
/// placeholder in OpenDmsConfig.cpp is replaced before flashing.
extern uint8_t OPENDMS_STATIC_PUBLIC_KEY[duckcrypto::PUBLIC_KEY_LENGTH];

/**
 * @brief Load a previously field-provisioned OpenDMS public key from
 * flash, if one exists, into OPENDMS_STATIC_PUBLIC_KEY.
 *
 * Safe to call even if none was ever provisioned (leaves the compile-time
 * placeholder/value untouched). Should be called once, early in setup.
 */
void begin();

/**
 * @brief Whether OPENDMS_STATIC_PUBLIC_KEY has been set to a real key.
 *
 * Returns false while the all-zero placeholder is still in place, meaning
 * encrypted downlink commands cannot be decrypted yet and should be
 * dropped rather than acted on.
 */
bool isConfigured();

/**
 * @brief Field-provision the OpenDMS static public key over the USB
 * serial console.
 *
 * Polls Serial for a complete line and, if present, handles one of:
 *  - "AT+OPENDMSKEY=<64 hex chars>": sets OPENDMS_STATIC_PUBLIC_KEY and
 *    persists it to flash. Only succeeds while isConfigured() is false,
 *    to prevent a stray write from silently reassigning an already
 *    provisioned device's org affiliation; send AT+OPENDMSKEY+RESET
 *    first to intentionally re-provision.
 *  - "AT+OPENDMSKEY+RESET": erases the stored key, resetting to the
 *    all-zero placeholder so a new AT+OPENDMSKEY= write will be
 *    accepted.
 *  - "AT+OPENDMSKEY?": prints whether a key is configured, and its value
 *    in hex (not secret, safe to print/log).
 * No authentication is required, deliberately: this value is not secret,
 * only provisioning convenience (see docs/crypto-design.tex, "Field
 * Operator Onboarding").
 *
 * Non-blocking: returns immediately if no complete line is available
 * yet. Intended to be called once per main loop iteration.
 */
void checkSerialProvisioning();

} // namespace opendmsconfig

#endif
