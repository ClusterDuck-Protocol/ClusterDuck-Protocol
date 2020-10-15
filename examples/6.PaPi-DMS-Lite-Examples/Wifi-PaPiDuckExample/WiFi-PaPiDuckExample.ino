/*
*
*
* PapaDuck configuration to be used with a PaPi/DMS-Lite device
*
*
*/

#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "timer.h"

// CDP headers
#include <PapaDuck.h>
#include <DuckDisplay.h>

// Note the device id here is just an example.
// It should be a unique securely stored ID
#define DEVICE_ID   "PapaDuck"

// Use pre-built papa duck.
PapaDuck duck = PapaDuck(DEVICE_ID);


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
void handleDuckData(Packet packet) {
  quackJson(packet);
  Serial.print("TEST");
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
//      Serial.print(client.state());
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
/**
 * @brief Convert received packet into a JSON object we can send over the MQTT connection
 * 
 * @param packet A Packet that contains the received message
 */
void quackJson(Packet packet) {
  if (packet.topic == "ping") {
    return;
  }
  const int bufferSize = 4*  JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;
  JsonObject root = doc.as<JsonObject>();
 
  doc["DeviceID"]  = packet.senderId;
  doc["MessageID"] = packet.messageId;
  doc["Payload"].set(packet.payload);
  // FIXME: Path shouldn't be altered by the application
  // It should done in the library PapaDuck component
  doc["path"].set(packet.path + "," + DEVICE_ID);
  String jsonstat;
  serializeJson(doc, jsonstat);
  if (mqttClient.publish(topic, jsonstat.c_str())) {
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("Publish ok");
  }
  else {
    Serial.println("Publish failed");
  }
}
