/**
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 * 
 * This is to be used with a PaPi/DMS-Lite device
 *
 * This example will configure and run a Papa Duck that connects to a local MQTT server
 * @date 2021-02-22
 *
 */

 /* CDP Headers */
#include <CdpPacket.h>
#include <PapaDuck.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <arduino-timer.h>
#include <string>

#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14



// Use pre-built papa duck.
PapaDuck duck = PapaDuck();

// create a timer with default settings
auto timer = timer_create_default();
char topic[] = "status";
const char* user = "raspi-webgui";
const char* pass = "ChangeMe";
const char* mqtt_server = "10.3.141.1";
const int MQTT_CONNECTION_DELAY_MS = 5000;
const int WIFI_CONNECTION_DELAY_MS = 500;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
 std::string deviceId("PAPADUCK");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  // the default setup is equivalent to the above setup sequence
// duck.setupSerial(115200);
  Serial.begin(115200);
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN,
                  LORA_DIO1_PIN, LORA_TXPOWER);
  duck.setDeviceId(devId);

  
  duck.onReceiveDuckData(handleDuckData);
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
  Serial.print("[PAPI] Setup OK!");
}

/**
 * @brief Callback method invoked when a packet was received from the mesh
 * 
 * @param packet data packet that contains the received message
 */
void handleDuckData(std::vector<byte> packetBuffer) {
  Serial.println("[PAPI] got packet: " +
                 convertToHex(packetBuffer.data(), packetBuffer.size()));
  quackJson(packetBuffer);
}

/**
 * @brief Establish the connection to the wifi network the Papa Duck can reach
 * 
 */
void setup_wifi() {
  Serial.println();
  Serial.print("[PAPI] Connecting to ");
  Serial.println(user);
  WiFi.begin(user, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFI_CONNECTION_DELAY_MS);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("[PAPI] WiFi connected");
  Serial.println("[PAPI] IP address: ");
}
/**
 * @brief Invoked by the MQTT server when a message has been received
 */
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("[PAPI] Message arrived on topic: ");
  Serial.print(topic);
}
/**
 * @brief Periodically attempts to re-establish the MQTT connection
 * 
 */
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[PAPI] Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("[PAPI] connected");
      mqttClient.subscribe("status");
    } else {
      Serial.print("[PAPI] failed, rc=");
      Serial.println("[PAPI] try again in 5 seconds");
      delay(MQTT_CONNECTION_DELAY_MS);
    }
  }
}
void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
   duck.run();
}

// DMS locator URL requires a topicString, so we need to convert the topic
// from the packet to a string based on the topics code
std::string toTopicString(byte topic) {

  std::string topicString;

  switch (topic) {
    case topics::status:
      topicString = "status";
      break;
    case topics::cpm:
      topicString = "portal";
      break;
    case topics::sensor:
      topicString = "sensor";
      break;
    case topics::alert:
      topicString = "alert";
      break;
    case topics::location:
      topicString = "gps";
      break;
    case topics::health:
      topicString = "health";
      break;
    default:
      topicString = "status";
  }

  return topicString;
}

/**
 * @brief Convert received packet into a JSON object we can send over the MQTT connection
 * 
 * @param packet A Packet that contains the received message
 */
void quackJson(std::vector<byte> packetBuffer) {

  CdpPacket packet = CdpPacket(packetBuffer);
  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  // Here we treat the internal payload of the CDP packet as a string
  // but this is mostly application dependent. 
  // The parsingf here is optional. The Papa duck could simply decide to
  // forward the CDP packet as a byte array and let the Network Server (or DMS) deal with
  // the parsing based on some business logic.

  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string dduid(packet.dduid.begin(), packet.dduid.end());

  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  Serial.println("[PAPI] Packet Received:");
  Serial.println("[PAPI] sduid:   " + String(sduid.c_str()));
  Serial.println("[PAPI] dduid:   " + String(dduid.c_str()));

  Serial.println("[PAPI] muid:    " + String(muid.c_str()));
  Serial.println("[PAPI] path:    " + String(path.c_str()));
  Serial.println("[PAPI] data:    " + String(payload.c_str()));
  Serial.println("[PAPI] hops:    " + String(packet.hopCount));
  Serial.println("[PAPI] duck:    " + String(packet.duckType));

  doc["DeviceID"] = sduid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["path"].set(path);
  doc["hops"].set(packet.hopCount);
  doc["duckType"].set(packet.duckType);

  std::string cdpTopic = toTopicString(packet.topic);

  std::string topic = "iot-2/evt/" + cdpTopic + "/fmt/json";

  String jsonstat;
  serializeJson(doc, jsonstat);

  if (mqttClient.publish(topic.c_str(), jsonstat.c_str())) {
    Serial.println("[PAPIDUCK] Packet forwarded:");
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("[PAPIDUCK] Publish ok");
  } else {
    Serial.println("[PAPIDUCK] Publish failed");
  }
}

String convertToHex(byte* data, int size) {
  String buf = "";
  buf.reserve(size * 2); // 2 digit hex
  const char* cs = "0123456789ABCDEF";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0F];
    buf += cs[val & 0x0F];
  }
  return buf;
}