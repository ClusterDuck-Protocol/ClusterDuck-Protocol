/**
 * @file RadioRegionConfig.h
 * @brief Runtime-selectable LoRa region/frequency preset, so a single
 * firmware build can be field-provisioned for the regulatory band of the
 * country it's deployed in, instead of requiring a per-country firmware
 * build matrix.
 *
 * Defaults to the MY (Malaysia) preset, which exactly matches this
 * repo's long-standing hardcoded values (CDPCFG_RF_LORA_FREQ = 922.8 MHz
 * mesh channel + the existing CDPCFG_UPLINK_CHANNEL_POOL) -- so an
 * unconfigured/legacy device's behavior does not change.
 *
 * A region can be set two ways, mirroring OpenDmsConfig.h/MeshGroupConfig.h:
 *  - field provisioning: begin() loads a previously field-provisioned
 *    region from flash if one was written via the serial provisioning
 *    command (see checkSerialProvisioning()), and applies it to
 *    DuckLoRa::defaultRadioParams.band and DuckLoRa::uplinkChannelPool
 *    before the radio is initialized.
 *  - a future BLE-writable characteristic/mobile app settings screen can
 *    call setRegion() directly at runtime.
 * If never configured, the compile-time MY preset (== the prior hardcoded
 * defaults) is used.
 *
 * IMPORTANT: the frequencies in RadioRegionConfig.cpp are representative
 * starting points reasoned from each region's general AS923/US915/EU868
 * regulatory plan, NOT verified against each country's current regulator
 * filing -- confirm against local regulations before flashing field
 * devices for a given country. Selecting a region does not itself enforce
 * that region's duty-cycle/dwell-time rules (e.g. EU868/UK's ~1% duty
 * cycle, US915 dwell-time/hopping) -- this firmware does not currently
 * enforce those for any region.
 *
 * @version
 * @date 2026-08-17
 *
 * @copyright
 */

#ifndef RADIOREGIONCONFIG_H_
#define RADIOREGIONCONFIG_H_

#include <stdint.h>
#include <string>

namespace radioregionconfig {

/// Supported regions. MY is the default/fallback (matches this repo's
/// long-standing hardcoded values).
enum class RadioRegion : uint8_t {
  MY = 0, ///< Malaysia (default -- unchanged from prior hardcoded values)
  SG = 1, ///< Singapore
  PH = 2, ///< Philippines
  ID = 3, ///< Indonesia
  US = 4, ///< United States (FCC 915 sub-band)
  UK = 5, ///< United Kingdom (ETSI EU868-style SRD band)
  PS = 6, ///< Palestine (ITU Region 1, ETSI EU868-style SRD band)
};

/// Number of channels in each region's uplink spreading pool.
constexpr uint8_t UPLINK_POOL_SIZE = 8;

/// Total number of supported regions (== RadioRegion::PS + 1).
constexpr uint8_t REGION_COUNT = 7;

/**
 * @brief Load a previously field-provisioned region from flash, if one
 * exists, and apply it to DuckLoRa::defaultRadioParams.band and
 * DuckLoRa::uplinkChannelPool. Applies the compile-time MY defaults if
 * none was ever provisioned (or storage failed an integrity check).
 * Should be called once, early in setup, before
 * setupLoRaRadio()/setupWithDefaults() initializes the radio.
 */
void begin();

/// Currently active region (MY until begin()/setRegion() loads or sets a
/// different one).
RadioRegion getCurrentRegion();

/// Short region code string (e.g. "MY", "US"), for logging/serial output.
const char* regionName(RadioRegion region);

/**
 * @brief Parse a short region code string (e.g. "MY", "US") into a
 * RadioRegion. Case-sensitive, matching the exact codes regionName()
 * returns. Shared by AT+RADIOREGION= serial provisioning and the BLE
 * CDK:RADIOREGION app frame (see example sketches' handleFrame()), so
 * both entry points recognize the same set of codes.
 *
 * @return true and sets *outRegion if `code` matched a known region,
 * false otherwise (*outRegion left untouched).
 */
bool regionFromName(const std::string& code, RadioRegion* outRegion);

/**
 * @brief Apply and persist a region preset to flash, updating
 * DuckLoRa::defaultRadioParams.band and DuckLoRa::uplinkChannelPool for
 * the *next* radio setup. Safe to call at runtime (e.g. from a BLE
 * settings write) as well as during setup.
 *
 * NOTE: this does not retune an already-initialized radio -- the SX12xx
 * chip was already configured with the previous band in
 * setupLoRaRadio()/setupWithDefaults(). A new mesh channel/uplink pool
 * only takes effect on air after a reboot, so callers (serial
 * AT+RADIOREGION= and the BLE CDK:RADIOREGION handler) auto-reboot the
 * device a short moment after a successful call here, giving the
 * serial/BLE response time to flush first.
 *
 * @return DUCK_ERR_NONE on success, an error code otherwise. On success,
 * the caller is expected to reboot the device shortly afterward.
 */
int setRegion(RadioRegion region);

/**
 * @brief Field-provision the LoRa region over the USB serial console.
 *
 * Polls Serial for a complete line and, if present, handles one of:
 *  - "AT+RADIOREGION=<code>": sets and persists the region, where
 *    <code> is one of MY, SG, PH, ID, US, UK, PS. Unlike OpenDmsConfig's key
 *    provisioning, this can be re-sent at any time to switch regions --
 *    a region choice is not a secret, so there's no need to fail closed
 *    or require a RESET first.
 *  - "AT+RADIOREGION+RESET": erases the field-provisioned region from
 *    flash and reverts to the compile-time MY default.
 *  - "AT+RADIOREGION?": logs the currently active region code.
 * A successful "AT+RADIOREGION=..." or "AT+RADIOREGION+RESET" auto-reboots
 * the device shortly afterward (see setRegion()) so the new mesh
 * channel/uplink pool takes effect immediately, without requiring the
 * operator to power-cycle the device manually.
 * Should be polled once per main loop iteration.
 */
void checkSerialProvisioning();

} // namespace radioregionconfig

#endif
