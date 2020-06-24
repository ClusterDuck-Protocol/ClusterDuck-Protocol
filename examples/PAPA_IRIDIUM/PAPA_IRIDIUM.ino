#include <ClusterDuck.h>
#include <PubSubClient.h>
#include <IridiumSBD.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "timer.h"

ClusterDuck duck;

auto timer = timer_create_default(); // create a timer with default settings

byte ping = 0xF4;

//============== IRIDIUM ================
#define IridiumSerial Serial2
#define DIAGNOSTICS false// Change this to see diagnostics
#define rxpin 25
#define txpin 13

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);
//============== IRIDIUM ================

//============== PAPADUCK ================
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

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);
//============== PAPADUCK ================

void setup() {
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("PapaDish");
  duck.setupDisplay("PapaDish");
  setupRockBlock();
  duck.setupLoRa();

  const char * ap = "PapaDuck Setup";
  duck.setupWifiAp(ap);
	duck.setupDns();
  duck.setupWebServer(true);

  duck.setupInternet(SSID, PASSWORD);
  setupMQTT();
  
//  Serial.println("PAPA Online");
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) setupMQTT();

  if(duck.getFlag()) {  //If LoRa packet received
    duck.flipFlag();
    duck.flipInterrupt();
    int pSize = duck.handlePacket();
    if(pSize > 3) {
      duck.getPacketData(pSize);
      if(WiFi.status() != WL_CONNECTED) {
        quackBeam();
      } else {
        quackJson();
      }
      
    }
    duck.flipInterrupt();
    duck.startReceive();
  }
  timer.tick();
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

void quackBeam() {
  int err;
  Packet lastPacket = duck.getLastPacket();
  
  //Uncomment the lines below of which data you want to send
  String data = lastPacket.senderId +  "/" + lastPacket.messageId + "/"+ lastPacket.payload + "/"+  lastPacket.path + "," + duck.getDeviceId() ;
  // String data = lastPacket.messageId ;
  
  Serial.println(data);
  const   char *text = data.c_str();
  // Send the message
  Serial.print("Trying to send the message.  This might take several minutes.\r\n");
  err = modem.sendSBDText(text);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("sendSBDText failed: error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Try again with a better view of the sky.");
  }
  else
  {
    Serial.println("Hey, it worked!");
  }
 
}

void setupRockBlock(){
  int signalQuality = -1;
  int err;
  
  // Start the serial port connected to the satellite modem
  Serial2.begin(19200, SERIAL_8N1, rxpin, txpin, false); // false means normal data polarity so steady state of line = 0, true means staedy state = high.
  // Begin satellite modem operation
  Serial.println("Starting modem...");
  err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print("Modem begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
    return;
  }
  // Example: Print the firmware revision
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
    Serial.print("FirmwareVersion failed: error ");
    Serial.println(err);
    return;
  }

  {
  Serial.print("Firmware Version is ");
  Serial.print(version);
  Serial.println(".");
  }
  
}
