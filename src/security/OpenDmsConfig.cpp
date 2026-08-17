#include "OpenDmsConfig.h"
#include <Arduino.h>
#include "../utils/DuckLogger.h"
#include "../utils/DuckError.h"

#include <cstring>
#include <string>

// Storage backend differs by platform, matching DuckIdentity.cpp: the
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

namespace opendmsconfig {

namespace {

// Storage layout: magic byte + 32B public key + 1B checksum. The
// checksum (see computeChecksum() below) is not a security boundary --
// it exists only to detect flash/file corruption of an already-trusted
// value, so a corrupted-but-nonzero key can't silently look "configured"
// while blackholing every encrypted_cmd (see isConfigured()'s doc
// comment in OpenDmsConfig.h).
// ESP32 (and any other/unknown platform): EEPROM.h, at OPENDMS_EEPROM_OFFSET.
// DuckWifi uses EEPROM.begin(512) for bytes 0-95 (SSID/password), and
// DuckIdentity uses offset 128, size 256 (occupying bytes [128, 384)) --
// this offset stays clear of both.
// nRF52: a dedicated file on the internal LittleFS filesystem, matching
// DuckIdentity's approach.
#if defined(ARDUINO_ARCH_NRF52)
constexpr char KEY_FILE_PATH[] = "/opendmskey.bin";
#else
constexpr int OPENDMS_EEPROM_OFFSET = 400;
constexpr int OPENDMS_EEPROM_SIZE = 64;
// IMPORTANT: always pass EEPROM_TOTAL_SIZE (not OPENDMS_EEPROM_SIZE +
// OPENDMS_EEPROM_OFFSET) to EEPROM.begin() below -- see the matching, more
// detailed comment in DuckIdentity.cpp's EEPROM_TOTAL_SIZE. In short:
// ESP32's EEPROM.h shares one NVS blob across DuckIdentity, OpenDmsConfig,
// MeshGroupConfig, DuckWifi and RadioRegionConfig, and calling begin()
// with a smaller size than what's currently stored permanently truncates
// (erases) every other module's data at higher offsets. Must stay in
// sync (same value) across all such files.
constexpr int EEPROM_TOTAL_SIZE = 536;
#endif
constexpr uint8_t KEY_MAGIC = 0xDA; // "Duck Agency key"

constexpr size_t KEY_HEX_LENGTH = duckcrypto::PUBLIC_KEY_LENGTH * 2;
// Generous bound on a serial provisioning line's length, well above the
// longest valid command ("AT+OPENDMSKEY=" + 64 hex chars), just to keep
// stray/unterminated line noise from growing this buffer unbounded.
constexpr size_t MAX_SERIAL_LINE_LENGTH = 128;

std::string serialLineBuffer;

// CRC-8-CCITT (poly 0x07) over the stored public key, used only to detect
// flash/file corruption -- NOT a security/authentication primitive (this
// key is not secret, see OpenDmsConfig.h). A mismatch means the stored
// bytes were altered by something other than saveToStorage() (bit rot,
// a failed/partial write, etc.), not a forgery -- an attacker able to
// overwrite this device's own flash could just as easily overwrite the
// checksum to match.
uint8_t computeChecksum(const uint8_t* key) {
  uint8_t crc = 0;
  for (size_t i = 0; i < duckcrypto::PUBLIC_KEY_LENGTH; i++) {
    crc ^= key[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

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
  file.read(outKey, duckcrypto::PUBLIC_KEY_LENGTH);
  uint8_t storedChecksum = 0;
  file.read(&storedChecksum, sizeof(storedChecksum));
  file.close();
  if (storedChecksum != computeChecksum(outKey)) {
    logerr_ln("OpenDmsConfig: stored key failed integrity check (checksum mismatch), "
              "treating as not configured -- storage may be corrupted; re-provision "
              "with AT+OPENDMSKEY+RESET then AT+OPENDMSKEY=...");
    return false;
  }
  return true;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  if (EEPROM.read(OPENDMS_EEPROM_OFFSET) != KEY_MAGIC) {
    return false;
  }
  for (size_t i = 0; i < duckcrypto::PUBLIC_KEY_LENGTH; i++) {
    outKey[i] = EEPROM.read(OPENDMS_EEPROM_OFFSET + 1 + i);
  }
  uint8_t storedChecksum = EEPROM.read(OPENDMS_EEPROM_OFFSET + 1 + duckcrypto::PUBLIC_KEY_LENGTH);
  if (storedChecksum != computeChecksum(outKey)) {
    logerr_ln("OpenDmsConfig: stored key failed integrity check (checksum mismatch), "
              "treating as not configured -- storage may be corrupted; re-provision "
              "with AT+OPENDMSKEY+RESET then AT+OPENDMSKEY=...");
    return false;
  }
  return true;
#endif
}

int saveToStorage(const uint8_t* key) {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(KEY_FILE_PATH, FILE_O_WRITE)) {
    logerr_ln("OpenDmsConfig: failed to open key file for writing");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  uint8_t magic = KEY_MAGIC;
  file.write(&magic, sizeof(magic));
  file.write(key, duckcrypto::PUBLIC_KEY_LENGTH);
  uint8_t checksum = computeChecksum(key);
  file.write(&checksum, sizeof(checksum));
  file.close();
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(OPENDMS_EEPROM_OFFSET, KEY_MAGIC);
  for (size_t i = 0; i < duckcrypto::PUBLIC_KEY_LENGTH; i++) {
    EEPROM.write(OPENDMS_EEPROM_OFFSET + 1 + i, key[i]);
  }
  EEPROM.write(OPENDMS_EEPROM_OFFSET + 1 + duckcrypto::PUBLIC_KEY_LENGTH, computeChecksum(key));
  if (!EEPROM.commit()) {
    logerr_ln("OpenDmsConfig: failed to commit key to storage");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

int eraseStorage() {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  if (InternalFS.exists(KEY_FILE_PATH) && !InternalFS.remove(KEY_FILE_PATH)) {
    logerr_ln("OpenDmsConfig: failed to erase stored key");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(OPENDMS_EEPROM_OFFSET, 0x00);
  if (!EEPROM.commit()) {
    logerr_ln("OpenDmsConfig: failed to erase stored key");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

// Decodes exactly KEY_HEX_LENGTH hex characters into
// duckcrypto::PUBLIC_KEY_LENGTH bytes. Returns false on any non-hex
// character or wrong length.
bool hexDecodeKey(const std::string& hex, uint8_t* outKey) {
  if (hex.size() != KEY_HEX_LENGTH) {
    return false;
  }
  for (size_t i = 0; i < duckcrypto::PUBLIC_KEY_LENGTH; i++) {
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
  for (size_t i = 0; i < duckcrypto::PUBLIC_KEY_LENGTH; i++) {
    out += digits[(key[i] >> 4) & 0x0F];
    out += digits[key[i] & 0x0F];
  }
  return out;
}

void handleSerialLine(const std::string& line) {
  static const std::string WRITE_PREFIX = "AT+OPENDMSKEY=";
  static const std::string RESET_CMD = "AT+OPENDMSKEY+RESET";
  static const std::string QUERY_CMD = "AT+OPENDMSKEY?";

  if (line == QUERY_CMD) {
    if (isConfigured()) {
      loginfo_ln("OpenDmsConfig: configured, public key = %s",
                 hexEncodeKey(OPENDMS_STATIC_PUBLIC_KEY).c_str());
    } else {
      loginfo_ln("OpenDmsConfig: not configured (all-zero placeholder)");
    }
    return;
  }

  if (line == RESET_CMD) {
    int rc = eraseStorage();
    if (rc == DUCK_ERR_NONE) {
      memset(OPENDMS_STATIC_PUBLIC_KEY, 0, duckcrypto::PUBLIC_KEY_LENGTH);
      loginfo_ln("OpenDmsConfig: key reset, ready for re-provisioning");
    }
    return;
  }

  if (line.rfind(WRITE_PREFIX, 0) == 0) {
    if (isConfigured()) {
      logerr_ln("OpenDmsConfig: already configured -- send "
                "AT+OPENDMSKEY+RESET first to re-provision");
      return;
    }
    std::string hex = line.substr(WRITE_PREFIX.size());
    uint8_t key[duckcrypto::PUBLIC_KEY_LENGTH];
    if (!hexDecodeKey(hex, key)) {
      logerr_ln("OpenDmsConfig: expected %u hex characters (got %u)",
                (unsigned)KEY_HEX_LENGTH, (unsigned)hex.size());
      return;
    }
    int rc = saveToStorage(key);
    if (rc == DUCK_ERR_NONE) {
      memcpy(OPENDMS_STATIC_PUBLIC_KEY, key, duckcrypto::PUBLIC_KEY_LENGTH);
      loginfo_ln("OpenDmsConfig: provisioned successfully");
    }
    return;
  }
}

} // namespace

uint8_t OPENDMS_STATIC_PUBLIC_KEY[duckcrypto::PUBLIC_KEY_LENGTH] = {0};

void begin() {
#if defined(OPENDMS_STATIC_PUBLIC_KEY_HEX)
  // Build-flag-provided key, e.g.
  // -DOPENDMS_STATIC_PUBLIC_KEY_HEX=\"<64 hex chars>\" -- see
  // tools/pubkey_to_c_array.py or `AT+OPENDMSKEY?` for the hex form of an
  // existing key. Applied first so field-provisioned storage (below) can
  // still override it if this device was later re-provisioned.
  if (!hexDecodeKey(OPENDMS_STATIC_PUBLIC_KEY_HEX, OPENDMS_STATIC_PUBLIC_KEY)) {
    logerr_ln("OpenDmsConfig: OPENDMS_STATIC_PUBLIC_KEY_HEX build flag is invalid "
              "(expected %u hex characters)", (unsigned)KEY_HEX_LENGTH);
  }
#endif

  uint8_t stored[duckcrypto::PUBLIC_KEY_LENGTH];
  if (loadFromStorage(stored)) {
    memcpy(OPENDMS_STATIC_PUBLIC_KEY, stored, duckcrypto::PUBLIC_KEY_LENGTH);
    loginfo_ln("OpenDmsConfig: loaded field-provisioned key from storage");
  }
}

bool isConfigured() {
  uint8_t zero[duckcrypto::PUBLIC_KEY_LENGTH] = {0};
  return memcmp(OPENDMS_STATIC_PUBLIC_KEY, zero, duckcrypto::PUBLIC_KEY_LENGTH) != 0;
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

} // namespace opendmsconfig

