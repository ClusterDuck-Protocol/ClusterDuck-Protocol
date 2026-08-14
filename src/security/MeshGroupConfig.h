/**
 * @file MeshGroupConfig.h
 * @brief Pins this deployment's pre-shared symmetric mesh group key, used
 * with duckcrypto::encryptWithGroupKey()/decryptWithGroupKey() to encrypt
 * local broadcast discovery traffic (e.g. BEACON/BEACON_ACK) that must be
 * readable by any Duck in the same deployment -- unlike session mode
 * (single known peer) or OpenDMS sealed mode (single fixed recipient),
 * neither of which fit a "readable by any/all nearby ducks" broadcast.
 *
 * MESH_GROUP_KEY ships as an all-zero placeholder, which isConfigured()
 * reports as "not configured" -- callers should fall back to sending the
 * discovery payload in the clear rather than fail closed, since discovery
 * availability matters more than confidentiality when the group key
 * hasn't been provisioned yet (unlike OpenDMS's downlink command channel,
 * where fail-closed is the safer default). It can be set three ways,
 * mirroring OpenDmsConfig.h:
 *  - build flag: pass -DMESH_GROUP_KEY_HEX="<64 hex chars>" and begin()
 *    decodes it into MESH_GROUP_KEY at startup.
 *  - compile-time: replace the initializer in MeshGroupConfig.cpp with
 *    the real deployment's key before flashing.
 *  - field provisioning: begin() loads a previously field-provisioned key
 *    from flash if one was written via the serial provisioning command
 *    (see checkSerialProvisioning()). Overrides the build-flag/compile-time
 *    value if present.
 * Unlike OpenDMS's static public key, this key IS secret -- anyone holding
 * it can both encrypt and decrypt group broadcast traffic -- so it should
 * be treated the same way as any other pre-shared symmetric secret when
 * provisioning devices in the field.
 *
 * @version
 * @date 2026-08-15
 *
 * @copyright
 */

#ifndef MESHGROUPCONFIG_H_
#define MESHGROUPCONFIG_H_

#include <stdint.h>
#include "DuckCrypto.h"

namespace meshgroupconfig {

/// This deployment's pre-shared mesh group symmetric key. All-zero until
/// begin() loads a field-provisioned key from flash, or the compile-time
/// placeholder in MeshGroupConfig.cpp is replaced before flashing.
extern uint8_t MESH_GROUP_KEY[duckcrypto::KEY_LENGTH];

/**
 * @brief Load a previously field-provisioned mesh group key from flash,
 * if one exists, into MESH_GROUP_KEY.
 *
 * Safe to call even if none was ever provisioned (leaves the compile-time
 * placeholder/value untouched). Should be called once, early in setup.
 */
void begin();

/**
 * @brief Whether MESH_GROUP_KEY has been set to a real key.
 *
 * Returns false while the all-zero placeholder is still in place, meaning
 * group-encrypted broadcast traffic cannot be sent/decrypted yet --
 * callers should fall back to sending in the clear rather than fail
 * closed (see file-level doc comment above).
 */
bool isConfigured();

/**
 * @brief Field-provision the mesh group key over the USB serial console.
 *
 * Polls Serial for a complete line and, if present, handles one of:
 *  - "AT+MESHKEY=<64 hex chars>": sets MESH_GROUP_KEY and persists it to
 *    flash. Only succeeds while isConfigured() is false, to prevent a
 *    stray write from silently reassigning an already provisioned
 *    device's group affiliation; send AT+MESHKEY+RESET first to
 *    re-provision.
 *  - "AT+MESHKEY+RESET": erases the field-provisioned key from flash and
 *    clears MESH_GROUP_KEY back to all-zero.
 *  - "AT+MESHKEY?": logs whether a key is currently configured, and if
 *    so, its hex form.
 * Should be polled once per main loop iteration.
 */
void checkSerialProvisioning();

} // namespace meshgroupconfig

#endif
