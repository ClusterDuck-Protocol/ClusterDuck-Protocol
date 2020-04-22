#include <ClusterDuck.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "timer.h"

#define SSID        ""
#define PASSWORD    ""

#define ORG         ""
#define DEVICE_ID   ""
#define DEVICE_TYPE ""
#define TOKEN       ""

char server[]           = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[]            = "iot-2/evt/status/fmt/json";
char authMethod[]       = "use-token-auth";
char token[]            = TOKEN;
char clientId[]         = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

ClusterDuck duck;

auto timer = timer_create_default(); // create a timer with default settings

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);

byte ping = 0xF4;

void setup() {
  // put your setup code here, to run once:

  duck.begin();
  duck.setDeviceId("Papa");

  duck.setupLoRa();
  duck.setupDisplay("Papa");

  const char * ap = "PapaDuck Setup";
  duck.setupWifiAp(ap);
	duck.setupDns();

	duck.setupInternet(SSID, PASSWORD);
  duck.setupWebServer();

  Serial.println("PAPA Online");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.print("WiFi disconnected, reconnecting to local network: ");
    Serial.print(duck.getSSID());
    duck.setupInternet(duck.getSSID(), duck.getPassword());
		duck.setupDns();
  }
  setupMQTT();

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


  timer.tick();
}



void setupMQTT()
{
  if (!!!client.connected()) {
    Serial.print("Reconnecting client to "); Serial.println(server);
    while ( ! (ORG == "quickstart" ? client.connect(clientId) : client.connect(clientId, authMethod, token)))
    {
      timer.tick(); //Advance timer to reboot after awhile
      Serial.print("i");
      delay(500);
    }
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