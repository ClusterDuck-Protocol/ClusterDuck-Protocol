/**
 * @file DMS-Lite-WiFi-PapaDuck.ino
 * @brief CDP 5.5 gateway for the DMS Lite Docker MQTT adapter.
 *
 * The Docker WiFi stack runs Mosquitto. This PapaDuck joins the same LAN and
 * publishes flat JSON to that broker on the packet's topic (sensor, health,
 * etc.). Set MQTT_SERVER to the Docker host's LAN IP, not a container address.
 * No internet service or TLS certificate is needed for the bundled broker.
 *
 * WiFi starts asynchronously. MQTT connection attempts have socket timeouts
 * and increasing retry delays; an attempt can still interrupt LoRa reception.
 * Up to 5 messages wait in RAM while MQTT is down; the oldest is dropped when
 * full. Messages are not durable and MQTT QoS 0 has no delivery acknowledgement.
 * See ../README-DMS-Lite.md for setup and the wire format.
 */

#include <algorithm>
#include <deque>
#include <string>
#include <CDP.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

const char DEVICE_ID[] = "PAPAMQTT";  // exactly 8 characters, unique in the mesh
const char WIFI_SSID[] = "";          // your 2.4 GHz WiFi network
const char WIFI_PASS[] = "";
const IPAddress MQTT_SERVER(192, 168, 1, 100);  // replace with Docker host's LAN IP
const uint16_t MQTT_PORT = 1883;               // MQTT_HOST_PORT in Docker's .env
const uint16_t MQTT_TIMEOUT_S = 2;
const uint16_t MQTT_BUFFER_SIZE = 2048;  // includes worst-case JSON escaping
const uint32_t MQTT_RETRY_MS = 5000;
const uint32_t MQTT_RETRY_MAX_MS = 60000;
const uint32_t WIFI_RETRY_MS = 30000;
const size_t QUEUE_SIZE_MAX = 5;
static_assert(sizeof(DEVICE_ID) == DUID_LENGTH + 1, "DEVICE_ID must be 8 characters");

struct PendingMessage {
  std::string topic;
  std::string json;
};

PapaDuck duck(DEVICE_ID);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
std::deque<PendingMessage> pending;
bool setupOK = false;
uint32_t lastWifiAttempt = 0;
uint32_t lastMqttAttempt = 0;
uint32_t mqttRetryMs = 0;  // first attempt as soon as WiFi connects

bool isValidUtf8(const std::vector<uint8_t>& bytes);
std::string packetToJson(CdpPacket& packet);
void handleDuckData(CdpPacket packet);
void serviceMqtt();

void setup() {
  const int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[DMS-LITE] Setup failed: %s\n", getDuckErrorString(err));
    return;
  }
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setSocketTimeout(MQTT_TIMEOUT_S);
  mqttClient.setKeepAlive(60);
  if (!mqttClient.setBufferSize(MQTT_BUFFER_SIZE)) {
    Serial.println("[DMS-LITE] MQTT buffer allocation failed");
    return;
  }
  duck.onReceiveDuckData(handleDuckData);
  setupOK = true;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lastWifiAttempt = millis();
  Serial.println("[DMS-LITE] MQTT gateway ready; connecting to WiFi");
}

void loop() {
  if (!setupOK) return;
  duck.run();
  serviceMqtt();
}

void handleDuckData(CdpPacket packet) {
  if (packet.topic < reservedTopic::max_reserved) return;
  std::string json = packetToJson(packet);
  if (json.empty()) return;
  if (pending.size() >= QUEUE_SIZE_MAX) {
    pending.pop_front();
    Serial.println("[DMS-LITE] MQTT queue full; oldest packet dropped");
  }
  pending.push_back({packet.topicToString(), std::move(json)});
}

void serviceMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    if (uint32_t(millis() - lastWifiAttempt) >= WIFI_RETRY_MS) {
      WiFi.reconnect();
      lastWifiAttempt = millis();
    }
    return;
  }

  if (!mqttClient.connected()) {
    if (uint32_t(millis() - lastMqttAttempt) < mqttRetryMs) return;
    // Use the numeric LAN IP and the explicit TCP timeout to avoid DNS waits.
    const bool connected = wifiClient.connect(MQTT_SERVER, MQTT_PORT, MQTT_TIMEOUT_S * 1000) &&
                           mqttClient.connect(DEVICE_ID);
    lastMqttAttempt = millis();
    if (!connected) {
      wifiClient.stop();
      mqttRetryMs = std::min(mqttRetryMs ? mqttRetryMs * 2 : MQTT_RETRY_MS, MQTT_RETRY_MAX_MS);
      Serial.printf("[DMS-LITE] MQTT unavailable; retry in %lu ms\n", (unsigned long)mqttRetryMs);
      return;
    }
    mqttRetryMs = MQTT_RETRY_MS;
    Serial.println("[DMS-LITE] MQTT connected");
  }

  mqttClient.loop();
  if (mqttClient.connected() && !pending.empty()) {
    // Drain one message per loop so reconnecting doesn't starve duck.run().
    const PendingMessage& message = pending.front();
    if (mqttClient.publish(message.topic.c_str(), message.json.c_str())) {
      pending.pop_front();
    } else {
      // Keep the message for a later attempt, with the same bounded queue.
      mqttClient.disconnect();
      lastMqttAttempt = millis();
      mqttRetryMs = MQTT_RETRY_MS;
      Serial.println("[DMS-LITE] MQTT publish failed; packet retained in RAM");
    }
  }
}

// Keep this serializer identical to the standalone serial example.
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
