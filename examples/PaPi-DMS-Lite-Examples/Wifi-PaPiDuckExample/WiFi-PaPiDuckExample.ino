/*
*
*
* PapaDuck configuration to be used with a PaPi/DMS-Lite device
*
*
*/

#include <ClusterDuck.h>

#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "timer.h"

ClusterDuck duck;

auto timer = timer_create_default(); // create a timer with default settings

char topic[] = "status";
const char* user = "raspi-webgui";
const char* pass = "ChangeMe";
const char* mqtt_server = "10.3.141.1";

WiFiClient espClient;
PubSubClient client(espClient);

byte ping = 0xF4;

void setup() {
  duck.begin();
  duck.setDeviceId("Papa");

  duck.setupLoRa();
  duck.setupDisplay("Papa");
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(user);
  WiFi.begin(user, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
}
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("status");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(duck.getFlag()) {  //If LoRa packet received
    duck.flipFlag();
    duck.flipInterrupt();
    int pSize = duck.handlePacket();
    if(pSize > 3) {
      String * msg = duck.getPacketData(pSize);
      quackJson();

    }
    duck.flipInterrupt();
    duck.startReceive();
  }
}
void quackJson() {
  const int bufferSize = 4*  JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  JsonObject root = doc.as<JsonObject>();

  Packet lastPacket = duck.getLastPacket();

  doc["DeviceID"]        = lastPacket.senderId;
  doc["MessageID"]       = lastPacket.messageId;
  doc["Payload"]     .set(lastPacket.payload);
  doc["path"]         .set(lastPacket.path + "," + duck.getDeviceId());


  String jsonstat;
  serializeJson(doc, jsonstat);

  if (client.publish(topic, jsonstat.c_str())) {

    serializeJsonPretty(doc, Serial);
     Serial.println("");
    Serial.println("Publish ok");

  }
  else {
    Serial.println("Publish failed");
  }

}