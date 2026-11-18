/**
 * @file DMS-Lite-Serial-PapaDuck.ino
 * @brief CDP 5.5 gateway for the DMS Lite Docker serial adapter.
 *
 * Connect this PapaDuck to the Docker host over USB. Each received application
 * packet, including this PapaDuck's own health, is one compact JSON line at
 * 115200 baud. Reserved CDP packets are omitted; relay copies are preserved.
 * CDP diagnostics are separate lines, which DMS Lite logs and skips.
 *
 * CDP 5.5 no longer handles the old /wifi/, /health/ or /channel/ commands.
 * Incoming serial commands are discarded without blocking the mesh.
 * See ../README-DMS-Lite.md for setup and the wire format.
 */

#include <algorithm>
#include <string>
#include <CDP.h>

const char DEVICE_ID[] = "PAPASER1";  // exactly 8 characters, unique in the mesh
static_assert(sizeof(DEVICE_ID) == DUID_LENGTH + 1, "DEVICE_ID must be 8 characters");

PapaDuck<DuckWifiNone> duck(DEVICE_ID);
bool setupOK = false;
bool warnedAboutCommands = false;

bool isValidUtf8(const std::vector<uint8_t>& bytes);
std::string packetToJson(CdpPacket& packet);
void handleDuckData(CdpPacket packet);

void setup() {
  const int err = duck.setupWithDefaults();  // includes Serial.begin(115200)
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[DMS-LITE] Setup failed: %s\n", getDuckErrorString(err));
    return;
  }
  duck.onReceiveDuckData(handleDuckData);
  setupOK = true;
  Serial.println("[DMS-LITE] Serial gateway ready at 115200 baud");
}

void loop() {
  if (!setupOK) return;
  duck.run();

  // The serial writer may send legacy commands without a newline. Do
  // not wait for framing or let an input stream starve LoRa processing.
  for (unsigned n = 0; n < 64 && Serial.available(); ++n) {
    Serial.read();
    if (!warnedAboutCommands) {
      Serial.println("[DMS-LITE] Legacy serial commands are unsupported in CDP 5.5");
      warnedAboutCommands = true;
    }
  }
}

void handleDuckData(CdpPacket packet) {
  if (packet.topic < reservedTopic::max_reserved) return;
  const std::string json = packetToJson(packet);
  if (!json.empty()) {
    Serial.println();  // finish any partial CDP diagnostic before the JSON line
    Serial.println(json.c_str());
  }
}

// Preserve UTF-8 text, including embedded NULs. Binary data uses explicit hex
// encoding so DMS Lite's strict UTF-8 decoder can still store every byte.
bool isValidUtf8(const std::vector<uint8_t>& bytes) {
  for (size_t i = 0; i < bytes.size();) {
    const uint8_t lead = bytes[i++];
    if (lead < 0x80) continue;
    unsigned count;
    uint32_t value, minimum;
    if (lead >= 0xC2 && lead <= 0xDF) {
      count = 1; value = lead & 0x1F; minimum = 0x80;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      count = 2; value = lead & 0x0F; minimum = 0x800;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      count = 3; value = lead & 0x07; minimum = 0x10000;
    } else {
      return false;
    }
    if (i + count > bytes.size()) return false;
    while (count--) {
      const uint8_t next = bytes[i++];
      if ((next & 0xC0) != 0x80) return false;
      value = (value << 6) | (next & 0x3F);
    }
    if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) return false;
  }
  return true;
}

std::string packetToJson(CdpPacket& packet) {
  JsonDocument doc;
  const bool printableId = std::all_of(packet.sduid.begin(), packet.sduid.end(),
    [](uint8_t c) { return c >= 0x21 && c <= 0x7E; });
  doc["DeviceID"] = printableId ? duckutils::duidToString(packet.sduid)
                                : duckutils::arrayToHexString(packet.sduid);
  if (!printableId) doc["DeviceIDEncoding"] = "hex";
  doc["MessageID"] = duckutils::arrayToHexString(packet.muid);
  doc["topic"] = packet.topicToString();
  if (isValidUtf8(packet.data)) {
    doc["Payload"] = std::string(packet.data.begin(), packet.data.end());
  } else {
    doc["Payload"] = "hex:" + duckutils::convertToHex(packet.data.data(), packet.data.size());
    doc["PayloadEncoding"] = "hex";
  }
  doc["path"] = "";  // required by DMS Lite; CDP 5.5 has no packet.path
  doc["hops"] = packet.hopCount;
  doc["duckType"] = packet.duckType;
  if (doc.overflowed()) {
    Serial.println("[DMS-LITE] JSON allocation failed; packet dropped");
    return {};
  }
  std::string json;
  serializeJson(doc, json);
  // ArduinoJson escapes common controls, but emits e.g. 0x01 literally. The
  // Python adapters require strict JSON, so escape any remaining control byte.
  std::string escaped;
  const char hex[] = "0123456789ABCDEF";
  for (unsigned char c : json) {
    if (c < 0x20) {
      escaped += "\\u00";
      escaped += hex[c >> 4];
      escaped += hex[c & 0x0F];
    } else {
      escaped += char(c);
    }
  }
  return escaped;
}
