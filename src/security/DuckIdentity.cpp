/**
 * @file DuckIdentity.cpp
 * @brief Implementation of this Duck's self-sovereign cryptographic identity.
 * See DuckIdentity.h and docs/crypto-design.tex for the design rationale.
 */

#include "DuckIdentity.h"
#include "../utils/DuckError.h"
#include "../utils/DuckLogger.h"

#include <Curve25519.h>
#include <RNG.h>
#include <SHA256.h>
#include <string.h>

// Storage backend differs by platform: the Adafruit nRF52 Arduino core
// (meshtastic fork) does not ship an EEPROM.h emulation, only
// Adafruit_LittleFS/InternalFileSystem (confirmed via actual build
// failure, see docs/crypto-design.tex). ESP32's arduino-esp32 core does
// ship EEPROM.h, so that path is kept there and used as the fallback for
// any other/unknown platform.
#if defined(ARDUINO_ARCH_NRF52)
#include <Adafruit_nRFCrypto.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;
#elif defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#include <esp_system.h>
#include <EEPROM.h>
#else
#include <EEPROM.h>
#endif

namespace duckidentity {

namespace {

// Storage layout: magic byte + 32B public key + 32B private key.
// ESP32 (and any other/unknown platform): EEPROM.h, starting at
// IDENTITY_EEPROM_OFFSET, chosen to stay clear of DuckWifi's SSID/password
// region (bytes 0-95, see src/wifi/DuckWifi.cpp) on boards where both are
// present.
// nRF52: a dedicated file on the internal LittleFS filesystem, since this
// core has no EEPROM.h emulation.
#if defined(ARDUINO_ARCH_NRF52)
constexpr char IDENTITY_FILE_PATH[] = "/duckidentity.bin";
#else
constexpr int IDENTITY_EEPROM_OFFSET = 128;
constexpr int IDENTITY_EEPROM_SIZE = 256;
// IMPORTANT: always pass EEPROM_TOTAL_SIZE (not IDENTITY_EEPROM_SIZE) to
// EEPROM.begin() below. On ESP32, EEPROM.h is backed by a single named NVS
// blob shared by every module that calls EEPROM.begin() (DuckIdentity,
// OpenDmsConfig, MeshGroupConfig, DuckWifi, RadioRegionConfig). If any
// module calls begin()
// with a size SMALLER than what's already stored, the ESP32 core's
// EEPROMClass::begin() immediately TRUNCATES the stored NVS blob down to
// that smaller size (nvs_set_blob with the shorter length) -- permanently
// discarding every other module's data living at higher offsets. Since
// duckidentity::begin() runs first every boot (see Duck::setupWithDefaults()),
// requesting only IDENTITY_EEPROM_SIZE (256) here was silently wiping
// OpenDmsConfig's (offset 400) and MeshGroupConfig's (offset 464) stored
// keys on every single reboot. EEPROM_TOTAL_SIZE must stay >= the highest
// (offset + size) used by ANY module sharing this NVS blob; bump it
// everywhere if the layout ever grows.
// Fixed layout: DuckWifi [0, 96), DuckIdentity [128, 384), OpenDmsConfig
// [400, 464), MeshGroupConfig [464, 528), RadioRegionConfig [528, 536).
// This must match the same constant in OpenDmsConfig.cpp,
// MeshGroupConfig.cpp, DuckWifi.cpp and RadioRegionConfig.cpp.
constexpr int EEPROM_TOTAL_SIZE = 536;
#endif
constexpr uint8_t IDENTITY_MAGIC = 0xDC; // "Duck Crypto"

uint8_t publicKey[PUBLIC_KEY_LENGTH];
uint8_t privateKey[PRIVATE_KEY_LENGTH];
bool initialized = false;

// Seed the vendored meshtastic/Crypto library's global CryptRNG with real
// hardware entropy before it is used for key generation.
//
// IMPORTANT: meshtastic/Crypto's RNG.cpp only has a built-in hardware TRNG
// mixed into CryptRNG.begin()/rand() for AVR, Arduino Due, ESP8266, ESP32
// and STM32WLE5xx. nRF52840 is not one of the platforms it knows about, so
// on that platform CryptRNG.begin() alone would only seed from a baked-in
// constant, an EEPROM-saved seed (if any), and micros() -- not real
// hardware randomness. We explicitly stir in bytes from the nRF52840's
// CryptoCell-310 hardware TRNG (via Adafruit_nRFCrypto) to compensate.
void seedCryptoRng() {
  CryptRNG.begin("meshbeacon-firmware DuckIdentity");

  uint8_t seed[32];
#if defined(ARDUINO_ARCH_NRF52)
  nRFCrypto.begin();
  nRFCrypto.Random.generate(seed, sizeof(seed));
#elif defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  // CryptRNG already mixes in esp_random() internally on ESP32, but stir
  // in an explicit hardware sample too for defense in depth.
  for (size_t i = 0; i < sizeof(seed); i += sizeof(uint32_t)) {
    uint32_t r = esp_random();
    memcpy(seed + i, &r, sizeof(r));
  }
#else
  logerr_ln("DuckIdentity: no hardware TRNG wired up for this platform; "
            "identity keys will be weaker than intended");
  memset(seed, 0, sizeof(seed));
#endif

  CryptRNG.stir(seed, sizeof(seed), sizeof(seed) * 8);
  memset(seed, 0, sizeof(seed));
}

bool loadFromStorage() {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(IDENTITY_FILE_PATH, FILE_O_READ)) {
    return false;
  }
  uint8_t magic = 0;
  file.read(&magic, sizeof(magic));
  if (magic != IDENTITY_MAGIC) {
    file.close();
    return false;
  }
  file.read(publicKey, PUBLIC_KEY_LENGTH);
  file.read(privateKey, PRIVATE_KEY_LENGTH);
  file.close();
  return true;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  if (EEPROM.read(IDENTITY_EEPROM_OFFSET) != IDENTITY_MAGIC) {
    return false;
  }
  for (size_t i = 0; i < PUBLIC_KEY_LENGTH; i++) {
    publicKey[i] = EEPROM.read(IDENTITY_EEPROM_OFFSET + 1 + i);
  }
  for (size_t i = 0; i < PRIVATE_KEY_LENGTH; i++) {
    privateKey[i] = EEPROM.read(IDENTITY_EEPROM_OFFSET + 1 + PUBLIC_KEY_LENGTH + i);
  }
  return true;
#endif
}

int saveToStorage() {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  File file(InternalFS);
  if (!file.open(IDENTITY_FILE_PATH, FILE_O_WRITE)) {
    logerr_ln("DuckIdentity: failed to open identity file for writing");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  uint8_t magic = IDENTITY_MAGIC;
  file.write(&magic, sizeof(magic));
  file.write(publicKey, PUBLIC_KEY_LENGTH);
  file.write(privateKey, PRIVATE_KEY_LENGTH);
  file.close();
  return DUCK_ERR_NONE;
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(IDENTITY_EEPROM_OFFSET, IDENTITY_MAGIC);
  for (size_t i = 0; i < PUBLIC_KEY_LENGTH; i++) {
    EEPROM.write(IDENTITY_EEPROM_OFFSET + 1 + i, publicKey[i]);
  }
  for (size_t i = 0; i < PRIVATE_KEY_LENGTH; i++) {
    EEPROM.write(IDENTITY_EEPROM_OFFSET + 1 + PUBLIC_KEY_LENGTH + i, privateKey[i]);
  }
  if (!EEPROM.commit()) {
    logerr_ln("DuckIdentity: failed to commit identity to storage");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
  return DUCK_ERR_NONE;
#endif
}

void generateKeyPair() {
  seedCryptoRng();
  // Curve25519::dh1() generates a fresh random private key ("f") using
  // CryptRNG and derives the matching public key ("k"). We reuse it purely
  // for X25519 keypair generation here, not as a live DH exchange.
  Curve25519::dh1(publicKey, privateKey);
}

} // namespace

int begin() {
  if (initialized) {
    return DUCK_ERR_NONE;
  }

  if (!loadFromStorage()) {
    loginfo_ln("DuckIdentity: no stored identity found, generating a new one");
    generateKeyPair();
    int rc = saveToStorage();
    if (rc != DUCK_ERR_NONE) {
      return rc;
    }
  }

  initialized = true;
  return DUCK_ERR_NONE;
}

const uint8_t* getPublicKey() {
  return publicKey;
}

const uint8_t* getPrivateKey() {
  return privateKey;
}

void getDuid(uint8_t* duid) {
  SHA256 sha;
  uint8_t hash[32];
  sha.reset();
  sha.update(publicKey, PUBLIC_KEY_LENGTH);
  sha.finalize(hash, sizeof(hash));
  memcpy(duid, hash, DUID_LENGTH);
}

int reset() {
#if defined(ARDUINO_ARCH_NRF52)
  InternalFS.begin();
  if (InternalFS.exists(IDENTITY_FILE_PATH) && !InternalFS.remove(IDENTITY_FILE_PATH)) {
    logerr_ln("DuckIdentity: failed to erase stored identity");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
#else
  EEPROM.begin(EEPROM_TOTAL_SIZE);
  EEPROM.write(IDENTITY_EEPROM_OFFSET, 0x00);
  if (!EEPROM.commit()) {
    logerr_ln("DuckIdentity: failed to erase stored identity");
    return DUCK_ERR_IDENTITY_STORAGE_WRITE;
  }
#endif
  initialized = false;
  memset(publicKey, 0, sizeof(publicKey));
  memset(privateKey, 0, sizeof(privateKey));
  return DUCK_ERR_NONE;
}

} // namespace duckidentity
