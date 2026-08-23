#include "RadioRegionConfig.h"
#include <Arduino.h>
#include "../utils/DuckLogger.h"
#include "../utils/DuckError.h"
#include "../DuckEsp.h"
#include "DuckLoRa.h"

#include <cstddef>
#include <cstring>
#include <string>

// Storage backend differs by platform, matching OpenDmsConfig.cpp/
// MeshGroupConfig.cpp: the Adafruit nRF52 Arduino core (meshtastic fork)
// does not ship an EEPROM.h emulation, only Adafruit_LittleFS/
// InternalFileSystem. ESP32's arduino-esp32 core does ship EEPROM.h, kept
// as the fallback for any other/unknown platform too.
#if defined(ARDUINO_ARCH_NRF52)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;
#else
#include <EEPROM.h>
#endif

namespace radioregionconfig {

namespace {

struct RadioRegionPreset {
  float meshChannelMHz;
  float uplinkChannelPoolMHz[UPLINK_POOL_SIZE];
};

// Representative per-region presets -- see the file-level doc comment in
// RadioRegionConfig.h for the "not regulator-verified" caveat. MY exactly
// matches the pre-existing CDPCFG_RF_LORA_FREQ / CDPCFG_UPLINK_CHANNEL_POOL
// values (922.8 MHz mesh channel, 922.8-921.4 MHz uplink pool) so an
// unconfigured device's behavior does not change. Array order must match
// the RadioRegion enum order in RadioRegionConfig.h.
constexpr RadioRegionPreset PRESETS[] = {
  /* MY */ {922.8f, {922.8f, 922.6f, 922.4f, 922.2f, 922.0f, 921.8f, 921.6f, 921.4f}},
  /* SG */ {923.2f, {923.2f, 923.0f, 922.8f, 922.6f, 922.4f, 922.2f, 922.0f, 921.8f}},
  /* PH */ {923.2f, {923.2f, 923.0f, 922.8f, 922.6f, 922.4f, 922.2f, 922.0f, 921.8f}},
  /* ID */ {916.8f, {916.8f, 916.6f, 916.4f, 916.2f, 916.0f, 915.8f, 915.6f, 915.4f}},
  /* US */ {915.2f, {915.2f, 915.0f, 914.8f, 914.6f, 914.4f, 914.2f, 914.0f, 913.8f}},
  /* UK */ {868.1f, {868.1f, 867.9f, 867.7f, 867.5f, 867.3f, 867.1f, 866.9f, 866.7f}},
  // Palestine has no dedicated LoRa sub-GHz ISM allocation of its own on
  // file; like most of ITU Region 1 lacking a country-specific plan, it
  // falls back to the common ETSI EU868 SRD860-870MHz band, so this
  // mirrors the UK preset's channel plan (see the file-level "not
  // regulator-verified" caveat in RadioRegionConfig.h).
  /* PS */ {868.1f, {868.1f, 867.9f, 867.7f, 867.5f, 867.3f, 867.1f, 866.9f, 866.7f}},
};

constexpr size_t PRESET_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);

RadioRegion currentRegion = RadioRegion::MY;

#if defined(ARDUINO_ARCH_NRF52)
constexpr char REGION_FILE_PATH[] = "/radioregion.bin";
#else
// DuckIdentity uses offset 128, size 256 (bytes [128, 384)); OpenDmsConfig
// uses offset 400, size 64 (bytes [400, 464)); MeshGroupConfig uses offset
// 464, size 64 (bytes [464, 528)) -- this offset (528) stays clear of all
// three.
constexpr int REGION_EEPROM_OFFSET = 528;
constexpr int REGION_EEPROM_SIZE = 8;
// IMPORTANT: always pass EEPROM_TOTAL_SIZE (not REGION_EEPROM_SIZE +
// REGION_EEPROM_OFFSET) to EEPROM.begin() below -- see the matching, more
// detailed comment in DuckIdentity.cpp's EEPROM_TOTAL_SIZE. In short:
// ESP32's EEPROM.h shares one NVS blob across DuckIdentity, OpenDmsConfig,
// MeshGroupConfig, DuckWifi and now RadioRegionConfig, and calling
// begin() with a smaller size than what's currently stored permanently
// truncates (erases) every other module's data at higher offsets. Must
// stay in sync (same value) across all such files.
constexpr int EEPROM_TOTAL_SIZE = 536;
#endif
// "Duck radio region" -- distinct from OpenDmsConfig's 0xDA,
// MeshGroupConfig's 0xDB and DuckIdentity's 0xDC.
constexpr uint8_t REGION_MAGIC = 0xDD;

// Generous bound on a serial provisioning line's length, well above the
// longest valid command ("AT+RADIOREGION=" + 2-char code).
constexpr size_t MAX_SERIAL_LINE_LENGTH = 64;

std::string serialLineBuffer;

// CRC-8-CCITT (poly 0x07) over the stored region id byte, used only to
// detect flash/file corruption -- NOT a security/authentication
// primitive, matching the pattern in DuckIdentity.cpp/OpenDmsConfig.cpp/
// MeshGroupConfig.cpp.
uint8_t computeChecksum(uint8_t regionId) {
  uint8_t crc = regionId;
  for (int b = 0; b < 8; b++) {
    crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
  }
  return crc;
}

bool isValidRegionId(uint8_t regionId) {
  return regionId < PRESET_COUNT;
}

void applyPreset(RadioRegion region) {
  const RadioRegionPreset& preset = PRESETS[static_cast<uint8_t>(region)];
  DuckLoRa::defaultRadioParams.band = preset.meshChannelMHz;
  for (uint8_t i = 0; i < UPLINK_POOL_SIZE; i++) {
    DuckLoRa::uplinkChannelPool[i] = preset.uplinkChannelPoolMHz[i];
  }
  currentRegion = region;
}

bool loadFromStorage(uint8_t* outRegionId) {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(REGION_FILE_PATH, FILE_O_READ)) {
    return false;
  }
  uint8_t magic = 0;
  file.read(&magic, sizeof(magic));
  if (magic != REGION_MAGIC) {
    file.close();
    return false;
  }
  uint8_t regionId = 0;
  file.read(&regionId, sizeof(regionId));
  uint8_t storedChecksum = 0;
  file.read(&storedChecksum, sizeof(storedChecksum));
  file.close();
  if (storedChecksum != computeChecksum(regionId) || !isValidRegionId(regionId)) {
    logerr_ln("RadioRegionConfig: stored region failed integrity check, "
              "falling back to default (MY) -- re-provision with "
              "AT+RADIOREGION=...");
    return false;
  }
  *outRegionId = regionId;
  return true;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  uint8_t magic = EEPROM.read(REGION_EEPROM_OFFSET);
  if (magic != REGION_MAGIC) {
    return false;
  }
  uint8_t regionId = EEPROM.read(REGION_EEPROM_OFFSET + 1);
  uint8_t storedChecksum = EEPROM.read(REGION_EEPROM_OFFSET + 2);
  if (storedChecksum != computeChecksum(regionId) || !isValidRegionId(regionId)) {
    logerr_ln("RadioRegionConfig: stored region failed integrity check, "
              "falling back to default (MY) -- re-provision with "
              "AT+RADIOREGION=...");
    return false;
  }
  *outRegionId = regionId;
  return true;
#endif
}

int saveToStorage(uint8_t regionId) {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(REGION_FILE_PATH, FILE_O_WRITE)) {
    logerr_ln("RadioRegionConfig: failed to open region file for writing");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  // FILE_O_WRITE opens in APPEND mode (seeks to EOF, does not truncate) --
  // without this, every re-provision after the first would append instead
  // of overwrite, so loadFromStorage() (which always reads from offset 0)
  // would keep returning the very first region ever set, no matter how
  // many times setRegion() is called afterward.
  file.truncate(0);
  file.seek(0);
  uint8_t magic = REGION_MAGIC;
  file.write(&magic, sizeof(magic));
  file.write(&regionId, sizeof(regionId));
  uint8_t checksum = computeChecksum(regionId);
  file.write(&checksum, sizeof(checksum));
  file.close();
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(REGION_EEPROM_OFFSET, REGION_MAGIC);
  EEPROM.write(REGION_EEPROM_OFFSET + 1, regionId);
  EEPROM.write(REGION_EEPROM_OFFSET + 2, computeChecksum(regionId));
  if (!EEPROM.commit()) {
    logerr_ln("RadioRegionConfig: failed to persist region");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

int eraseStorage() {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  if (InternalFS.exists(REGION_FILE_PATH) && !InternalFS.remove(REGION_FILE_PATH)) {
    logerr_ln("RadioRegionConfig: failed to erase stored region");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(REGION_EEPROM_OFFSET, 0x00);
  if (!EEPROM.commit()) {
    logerr_ln("RadioRegionConfig: failed to erase stored region");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

void handleSerialLine(const std::string& line) {
  static const std::string WRITE_PREFIX = "AT+RADIOREGION=";
  static const std::string RESET_CMD = "AT+RADIOREGION+RESET";
  static const std::string QUERY_CMD = "AT+RADIOREGION?";

  if (line == QUERY_CMD) {
    loginfo_ln("RadioRegionConfig: current region = %s", regionName(currentRegion));
    return;
  }

  if (line == RESET_CMD) {
    int rc = eraseStorage();
    if (rc == DUCK_ERR_NONE) {
      applyPreset(RadioRegion::MY);
      loginfo_ln("RadioRegionConfig: reset to default region (MY), rebooting...");
      Serial.flush();
      delay(300);  // let the serial response flush before resetting
      duckesp::restartDuck();
    }
    return;
  }

  if (line.rfind(WRITE_PREFIX, 0) == 0) {
    std::string code = line.substr(WRITE_PREFIX.size());
    RadioRegion region;
    if (!regionFromName(code, &region)) {
      logerr_ln("RadioRegionConfig: unrecognized region code '%s'", code.c_str());
      return;
    }
    int rc = setRegion(region);
    if (rc == DUCK_ERR_NONE) {
      loginfo_ln("RadioRegionConfig: region set to %s, rebooting...", regionName(region));
      Serial.flush();
      delay(300);  // let the serial response flush before resetting
      duckesp::restartDuck();
    }
    return;
  }
}

} // namespace

void begin() {
  uint8_t storedRegionId;
  if (loadFromStorage(&storedRegionId)) {
    applyPreset(static_cast<RadioRegion>(storedRegionId));
    loginfo_ln("RadioRegionConfig: loaded field-provisioned region %s from storage",
               regionName(currentRegion));
  } else {
    applyPreset(RadioRegion::MY);
  }
}

RadioRegion getCurrentRegion() {
  return currentRegion;
}

const char* regionName(RadioRegion region) {
  switch (region) {
    case RadioRegion::MY: return "MY";
    case RadioRegion::SG: return "SG";
    case RadioRegion::PH: return "PH";
    case RadioRegion::ID: return "ID";
    case RadioRegion::US: return "US";
    case RadioRegion::UK: return "UK";
    case RadioRegion::PSE: return "PS";
  }
  return "MY";
}

int setRegion(RadioRegion region) {
  int rc = saveToStorage(static_cast<uint8_t>(region));
  if (rc == DUCK_ERR_NONE) {
    applyPreset(region);
  }
  return rc;
}

bool regionFromName(const std::string& code, RadioRegion* outRegion) {
  for (uint8_t i = 0; i < REGION_COUNT; i++) {
    if (code == regionName(static_cast<RadioRegion>(i))) {
      *outRegion = static_cast<RadioRegion>(i);
      return true;
    }
  }
  return false;
}

void checkSerialProvisioning() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (!serialLineBuffer.empty()) {
        handleSerialLine(serialLineBuffer);
        serialLineBuffer.clear();
      }
    } else {
      serialLineBuffer += c;
      if (serialLineBuffer.size() > MAX_SERIAL_LINE_LENGTH) {
        serialLineBuffer.clear();
      }
    }
  }
}

} // namespace radioregionconfig
