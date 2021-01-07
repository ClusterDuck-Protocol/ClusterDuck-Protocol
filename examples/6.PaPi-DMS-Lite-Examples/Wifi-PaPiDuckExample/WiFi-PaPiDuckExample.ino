/**
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 * 
 * This is to be used with a PaPi/DMS-Lite device
 *
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud.
 *
 * @date 2020-09-21
 *
 */
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <arduino-timer.h>
#include <string>

/* CDP Headers */
#include <CdpPacket.h>
#include <PapaDuck.h>

// Note the device id here is just an example.
// It should be a unique securely stored ID
const std::string DEVICE_ID = "PAPA0001";

// Use pre-built papa duck.
PapaDuck duck = PapaDuck();


// OLED display
DuckDisplay* display = NULL;

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
  duck.setupSerial();
  duck.setupRadio();
  duck.setDeviceId((byte*)DEVICE_ID.data());
  duck.onReceiveDuckData(handleDuckData);
  // This duck has an OLED display and we want to use it. 
  // Get an instance and initialize it, so we can use in our application
  // Note: if a display is not available, we still get a valid DuckDisplay
  // but the API calls will be a no-op
  display = DuckDisplay::getInstance();
  display->setupDisplay();
  display->drawString(true, 10,10, "setup wifi");
  setup_wifi();
  display->drawString(true, 10,10, "setup mqtt");
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
  display->drawString(true, 10,10, "PAPA READY");
}

/**
 * @brief Callback method invoked when a packet was received from the mesh
 * 
 * @param packet data packet that contains the received message
 */
void handleDuckData(CDP_Packet packet) {
  // is ping the only reserved message we want to ignore?
  // Maybe packet.topic < reservedTopic::max_reserved is better here
  if (packet.topic == reservedTopic::ping) {
     return;
  }
  Serial.print("Got some data...");
  quackJson(packet);
}

/**
 * @brief Establish the connection to the wifi network the Papa Duck can reach
 * 
 */
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(user);
  WiFi.begin(user, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFI_CONNECTION_DELAY_MS);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
}
/**
 * @brief Invoked by the MQTT server when a message has been received
 */
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
}
/**
 * @brief Periodically attempts to re-establish the MQTT connection
 * 
 */
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("connected");
      mqttClient.subscribe("status");
    } else {
      Serial.print("failed, rc=");
      Serial.println(" try again in 5 seconds");
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
void quackJson(CDP_Packet packet) {
  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  std::string payload(packet.data.begin(), packet.data.end());
  std::string duid(packet.sduid.begin(), packet.sduid.end());
  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  doc["DeviceID"] = duid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["path"].set(path);

  std::string cdpTopic = toTopicString(packet.topic);

  std::string topic = "iot-2/evt/" + cdpTopic + "/fmt/json";

  String jsonstat;
  serializeJson(doc, jsonstat);

  if (client.publish(topic.c_str(), jsonstat.c_str())) {
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("Publish ok");
  } else {
    Serial.println("Publish failed");
  }
}
