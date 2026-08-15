#include "MeshGroupConfig.h"
#include <new>
#include <Arduino.h>
#include "../utils/DuckLogger.h"
#include "../utils/DuckError.h"

#include <cstring>
#include <string>

// Storage backend differs by platform, matching OpenDmsConfig.cpp: the
// Adafruit nRF52 Arduino core (meshtastic fork) does not ship an EEPROM.h
// emulation, only Adafruit_LittleFS/InternalFileSystem. ESP32's
// arduino-esp32 core does ship EEPROM.h, kept as the fallback for any
// other/unknown platform too.
#if defined(ARDUINO_ARCH_NRF52)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;
#else
#include <EEPROM.h>
#endif

namespace meshgroupconfig {

namespace {

// Storage layout: magic byte + 32B key.
// ESP32 (and any other/unknown platform): EEPROM.h, at MESH_EEPROM_OFFSET.
// DuckIdentity uses offset 128, size 256 (bytes [128, 384)); OpenDmsConfig
// uses offset 400, size 64 (bytes [400, 464)) -- this offset (464) stays
// clear of both.
// nRF52: a dedicated file on the internal LittleFS filesystem, matching
// OpenDmsConfig's approach.
#if defined(ARDUINO_ARCH_NRF52)
constexpr char KEY_FILE_PATH[] = "/meshgroupkey.bin";
#else
constexpr int MESH_EEPROM_OFFSET = 464;
constexpr int MESH_EEPROM_SIZE = 64;
// IMPORTANT: always pass EEPROM_TOTAL_SIZE (not MESH_EEPROM_SIZE +
// MESH_EEPROM_OFFSET) to EEPROM.begin() below -- see the matching, more
// detailed comment in DuckIdentity.cpp's EEPROM_TOTAL_SIZE. In short:
// ESP32's EEPROM.h shares one NVS blob across DuckIdentity, OpenDmsConfig,
// MeshGroupConfig and DuckWifi, and calling begin() with a smaller size
// than what's currently stored permanently truncates (erases) every other
// module's data at higher offsets. Must stay in sync (same value) across
// all four files.
constexpr int EEPROM_TOTAL_SIZE = 528;
#endif
constexpr uint8_t KEY_MAGIC = 0xDB; // "Duck group key" (0xDA is OpenDmsConfig's)

constexpr size_t KEY_HEX_LENGTH = duckcrypto::KEY_LENGTH * 2;
// Generous bound on a serial provisioning line's length, well above the
// longest valid command ("AT+MESHKEY=" + 64 hex chars), just to keep
// stray/unterminated line noise from growing this buffer unbounded.
constexpr size_t MAX_SERIAL_LINE_LENGTH = 128;

std::string serialLineBuffer;

bool loadFromStorage(uint8_t* outKey) {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(KEY_FILE_PATH, FILE_O_READ)) {
    return false;
  }
  uint8_t magic = 0;
  file.read(&magic, sizeof(magic));
  if (magic != KEY_MAGIC) {
    file.close();
    return false;
  }
  file.read(outKey, duckcrypto::KEY_LENGTH);
  file.close();
  return true;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  if (EEPROM.read(MESH_EEPROM_OFFSET) != KEY_MAGIC) {
    return false;
  }
  for (size_t i = 0; i < duckcrypto::KEY_LENGTH; i++) {
    outKey[i] = EEPROM.read(MESH_EEPROM_OFFSET + 1 + i);
  }
  return true;
#endif
}

int saveToStorage(const uint8_t* key) {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(KEY_FILE_PATH, FILE_O_WRITE)) {
    logerr_ln("MeshGroupConfig: failed to open key file for writing");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  uint8_t magic = KEY_MAGIC;
  file.write(&magic, sizeof(magic));
  file.write(key, duckcrypto::KEY_LENGTH);
  file.close();
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(MESH_EEPROM_OFFSET, KEY_MAGIC);
  for (size_t i = 0; i < duckcrypto::KEY_LENGTH; i++) {
    EEPROM.write(MESH_EEPROM_OFFSET + 1 + i, key[i]);
  }
  if (!EEPROM.commit()) {
    logerr_ln("MeshGroupConfig: failed to commit key to storage");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

int eraseStorage() {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  if (InternalFS.exists(KEY_FILE_PATH) && !InternalFS.remove(KEY_FILE_PATH)) {
    logerr_ln("MeshGroupConfig: failed to erase stored key");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(MESH_EEPROM_OFFSET, 0x00);
  if (!EEPROM.commit()) {
    logerr_ln("MeshGroupConfig: failed to erase stored key");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

// Decodes exactly KEY_HEX_LENGTH hex characters into
// duckcrypto::KEY_LENGTH bytes. Returns false on any non-hex character or
// wrong length.
bool hexDecodeKey(const std::string& hex, uint8_t* outKey) {
  if (hex.size() != KEY_HEX_LENGTH) {
    return false;
  }
  for (size_t i = 0; i < duckcrypto::KEY_LENGTH; i++) {
    char hi = hex[i * 2];
    char lo = hex[i * 2 + 1];
    int hiVal, loVal;
    if (hi >= '0' && hi <= '9') hiVal = hi - '0';
    else if (hi >= 'a' && hi <= 'f') hiVal = hi - 'a' + 10;
    else if (hi >= 'A' && hi <= 'F') hiVal = hi - 'A' + 10;
    else return false;
    if (lo >= '0' && lo <= '9') loVal = lo - '0';
    else if (lo >= 'a' && lo <= 'f') loVal = lo - 'a' + 10;
    else if (lo >= 'A' && lo <= 'F') loVal = lo - 'A' + 10;
    else return false;
    outKey[i] = static_cast<uint8_t>((hiVal << 4) | loVal);
  }
  return true;
}

std::string hexEncodeKey(const uint8_t* key) {
  static const char* digits = "0123456789abcdef";
  std::string out;
  out.reserve(KEY_HEX_LENGTH);
  for (size_t i = 0; i < duckcrypto::KEY_LENGTH; i++) {
    out += digits[(key[i] >> 4) & 0x0F];
    out += digits[key[i] & 0x0F];
  }
  return out;
}

void handleSerialLine(const std::string& line) {
  static const std::string WRITE_PREFIX = "AT+MESHKEY=";
  static const std::string RESET_CMD = "AT+MESHKEY+RESET";
  static const std::string QUERY_CMD = "AT+MESHKEY?";

  if (line == QUERY_CMD) {
    if (isConfigured()) {
      loginfo_ln("MeshGroupConfig: configured, key = %s",
                 hexEncodeKey(MESH_GROUP_KEY).c_str());
    } else {
      loginfo_ln("MeshGroupConfig: not configured (all-zero placeholder)");
    }
    return;
  }

  if (line == RESET_CMD) {
    int rc = eraseStorage();
    if (rc == DUCK_ERR_NONE) {
      memset(MESH_GROUP_KEY, 0, duckcrypto::KEY_LENGTH);
      loginfo_ln("MeshGroupConfig: key reset, ready for re-provisioning");
    }
    return;
  }

  if (line.rfind(WRITE_PREFIX, 0) == 0) {
    if (isConfigured()) {
      logerr_ln("MeshGroupConfig: already configured -- send "
                "AT+MESHKEY+RESET first to re-provision");
      return;
    }
    std::string hex = line.substr(WRITE_PREFIX.size());
    uint8_t key[duckcrypto::KEY_LENGTH];
    if (!hexDecodeKey(hex, key)) {
      logerr_ln("MeshGroupConfig: expected %u hex characters (got %u)",
                (unsigned)KEY_HEX_LENGTH, (unsigned)hex.size());
      return;
    }
    int rc = saveToStorage(key);
    if (rc == DUCK_ERR_NONE) {
      memcpy(MESH_GROUP_KEY, key, duckcrypto::KEY_LENGTH);
      loginfo_ln("MeshGroupConfig: provisioned successfully");
    }
    return;
  }
}

} // namespace

uint8_t MESH_GROUP_KEY[duckcrypto::KEY_LENGTH] = {0};

void begin() {
#if defined(MESH_GROUP_KEY_HEX)
  // Build-flag-provided key, e.g. -DMESH_GROUP_KEY_HEX=\"<64 hex chars>\".
  // Applied first so field-provisioned storage (below) can still override
  // it if this device was later re-provisioned.
  if (!hexDecodeKey(MESH_GROUP_KEY_HEX, MESH_GROUP_KEY)) {
    logerr_ln("MeshGroupConfig: MESH_GROUP_KEY_HEX build flag is invalid "
              "(expected %u hex characters)", (unsigned)KEY_HEX_LENGTH);
  }
#endif

  uint8_t stored[duckcrypto::KEY_LENGTH];
  if (loadFromStorage(stored)) {
    memcpy(MESH_GROUP_KEY, stored, duckcrypto::KEY_LENGTH);
    loginfo_ln("MeshGroupConfig: loaded field-provisioned key from storage");
  }
}

bool isConfigured() {
  uint8_t zero[duckcrypto::KEY_LENGTH] = {0};
  return memcmp(MESH_GROUP_KEY, zero, duckcrypto::KEY_LENGTH) != 0;
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

} // namespace meshgroupconfig
