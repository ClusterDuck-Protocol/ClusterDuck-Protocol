/**
 * @file PapaDuck.ino
 * @brief A PapaDuck that forwards CDP messages to an MQTT broker over WiFi.
 *
 * The PapaDuck is the gateway of the mesh: ducks address their data to it by
 * default. This sketch joins a WiFi network, connects to an MQTT broker over
 * TLS (test.mosquitto.org by default) and publishes every message the duck
 * receives as JSON on MQTT_PUB_TOPIC. It also subscribes to MQTT_SUB_TOPIC and
 * prints what arrives there. web/ holds a small browser page that shows the
 * published messages.
 *
 * - The receive callback gets every copy of a message: one per relay path that
 *   reached this duck. This sketch publishes every copy; remove duplicates
 *   downstream by MessageID, or call duck.muidNotReceived(packet.muid) in the
 *   callback to drop them here.
 * - Every 15 minutes the PapaDuck's own health (topics::health) also arrives in
 *   the callback, as a packet from this duck with hop count 0. It doesn't go
 *   over LoRa. This sketch publishes it like the health of any other duck.
 * - Pings, pongs and other reserved topics are not published.
 * - While it connects to the broker, the PapaDuck doesn't receive, and LoRa
 *   packets that arrive then can be lost. The MQTT_TIMEOUT_S settings keep each
 *   attempt short, and after each failed attempt it waits twice as long before
 *   the next one, up to MQTT_RETRY_MAX_MS. WiFi reconnects by itself.
 * - WiFi, TLS and MQTT don't fit the default 1.25 MB app partition. The root
 *   platformio.ini sets board_build.partitions = no_ota.csv; use the same in
 *   your own PlatformIO project.
 */

#include <algorithm>
#include <string>
#include <CDP.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// --- Configuration ---
const char DEVICE_ID[] = "PAPADUCK";            // exactly 8 characters, unique in the mesh
const std::string WIFI_SSID = "";               // your WiFi network (2.4 GHz)
const std::string WIFI_PASS = "";               // its password

const char MQTT_SERVER[] = "test.mosquitto.org";
const uint16_t MQTT_PORT = 8883;                // TLS
const char MQTT_CLIENT_ID[] = "papa-duck-mqtt-1";  // must be unique on the broker
const char MQTT_PUB_TOPIC[] = "hub/event";        // where this PapaDuck publishes
const char MQTT_SUB_TOPIC[] = "incoming/say_hello";  // what it subscribes to
const unsigned long MQTT_CHECK_MS = 5000;       // time between two connection checks
const unsigned long MQTT_RETRY_MAX_MS = 60000;  // longest wait after failed attempts
const uint16_t MQTT_TIMEOUT_S = 5;              // longest wait for each connection step
// PubSubClient's default 256-byte buffer is too small for a 228-byte CDP
// payload wrapped in JSON: publish() returns false when a message doesn't fit.
// JSON writes a control byte, such as 0x00, as 6 characters, so a binary
// payload can need about 1.6 KB.
const uint16_t MQTT_BUFFER_SIZE = 2048;

// CA certificate of test.mosquitto.org, from https://test.mosquitto.org/
static const char MQTT_CA_CERT[] = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n" \
"BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\n" \
"A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\n" \
"BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\n" \
"by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\n" \
"BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\n" \
"MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\n" \
"dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\n" \
"KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\n" \
"UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\n" \
"Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\n" \
"s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\n" \
"3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\n" \
"E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\n" \
"MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\n" \
"6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n" \
"BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\n" \
"6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\n" \
"+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\n" \
"sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\n" \
"LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\n" \
"m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n" \
"-----END CERTIFICATE-----\n";


// --- Globals ---
PapaDuck duck(DEVICE_ID);
auto timer = timer_create_default();
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
bool setupOK = false;
unsigned long mqttRetryMs = MQTT_CHECK_MS;   // wait before the next connection check

void handleDuckData(CdpPacket packet);
void handleMqttMessage(char* topic, byte* payload, unsigned int length);
bool connectMqtt(void*);
void publishPacket(CdpPacket& packet);

void setup() {
  // Starts the serial port at 115200 baud and the LoRa radio.
  int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[PAPA] Setup failed, rc = %d (%s)\n", err, getDuckErrorString(err));
    return;
  }
  duck.onReceiveDuckData(handleDuckData);
  setupOK = true;

  // Retries a few times when the network isn't found, so this can take ~30 s.
  // After that, the ESP32 reconnects to WiFi by itself.
  duck.joinWifiNetwork(WIFI_SSID, WIFI_PASS);

  wifiClient.setCACert(MQTT_CA_CERT);
#if ESP_ARDUINO_VERSION_MAJOR < 3
  wifiClient.setTimeout(MQTT_TIMEOUT_S);           // TCP connect, in seconds (ESP32 core 2.x)
#endif
  wifiClient.setHandshakeTimeout(MQTT_TIMEOUT_S);  // TLS handshake, in seconds
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(handleMqttMessage);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(MQTT_TIMEOUT_S);     // wait for the broker's answer
  connectMqtt(nullptr);   // connects, and schedules the next check

  Serial.println("[PAPA] Setup OK");
}

void loop() {
  if (!setupOK) {
    return;
  }
  duck.run();         // receives packets and calls handleDuckData()
  timer.tick();
  mqttClient.loop();  // keeps the MQTT connection alive and calls handleMqttMessage()
}

/**
 * @brief Called by duck.run() for every packet the PapaDuck receives, and for
 * its own health message.
 */
void handleDuckData(CdpPacket packet) {
  if (packet.topic < reservedTopic::max_reserved) {
    return;   // a reserved topic, such as a ping or pong
  }
  publishPacket(packet);
}

/**
 * @brief Connect to the MQTT broker if WiFi is up and MQTT isn't, then
 * schedule the next check. Called once from setup() and then by the timer.
 * Each failed attempt doubles the wait before the next one, up to
 * MQTT_RETRY_MAX_MS, because duck.run() doesn't run during an attempt.
 * @returns false: this is a one-shot timer, and the next one is set here
 */
bool connectMqtt(void*) {
  if (!mqttClient.connected() && duck.isWifiConnected()) {
    Serial.printf("[PAPA] Connecting to MQTT broker %s\n", MQTT_SERVER);
    // For a broker that needs a login: mqttClient.connect(MQTT_CLIENT_ID, user, password)
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("[PAPA] MQTT connected");
      mqttRetryMs = MQTT_CHECK_MS;
      if (!mqttClient.subscribe(MQTT_SUB_TOPIC)) {
        Serial.printf("[PAPA] Failed to subscribe to %s\n", MQTT_SUB_TOPIC);
      }
    } else {
      mqttRetryMs = std::min(2 * mqttRetryMs, MQTT_RETRY_MAX_MS);
      Serial.printf("[PAPA] MQTT connect failed, state = %d, next attempt in %lu s\n",
        mqttClient.state(), mqttRetryMs / 1000);
    }
  }
  timer.in(mqttRetryMs, connectMqtt);
  return false;
}

/**
 * @brief Called by mqttClient.loop() for each message on MQTT_SUB_TOPIC.
 */
void handleMqttMessage(char* topic, byte* payload, unsigned int length) {
  std::string message(reinterpret_cast<const char*>(payload), length);
  Serial.printf("[PAPA] MQTT message on %s: %s\n", topic, message.c_str());
  // Act on messages from the cloud here.
}

/**
 * @brief Publish one CDP packet as JSON on MQTT_PUB_TOPIC.
 */
void publishPacket(CdpPacket& packet) {
  JsonDocument doc;
  doc["from"] = "hub";
  doc["to"] = "controller";
  doc["RE"] = false;   // whether the controller should reply
  doc["eventType"] = packet.topicToString();
  doc["MessageID"] = duckutils::toString(packet.muid);
  doc["payload"]["hops"] = packet.hopCount;
  doc["payload"]["duckType"] = packet.duckType;
  doc["payload"]["DeviceID"] = duckutils::toString(packet.sduid);
  doc["payload"]["Message"] = std::string(packet.data.begin(), packet.data.end());

  std::string json;
  serializeJson(doc, json);
  Serial.printf("[PAPA] Received: %s\n", json.c_str());

  if (!mqttClient.connected()) {
    Serial.println("[PAPA] MQTT not connected, message not published");
    return;
  }
  if (mqttClient.publish(MQTT_PUB_TOPIC, json.c_str())) {
    Serial.println("[PAPA] Published");
  } else {
    Serial.println("[PAPA] Publish failed");
  }
}
