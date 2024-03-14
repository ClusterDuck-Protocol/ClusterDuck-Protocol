/**
 * @file PapaDuck.ino
 * @author Timo Wielink
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 *
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud. When disconnected
 * it will add received packets to a queue. When it reconnects to MQTT it will
 * try to publish all messages in the queue. You can change the size of the queue
 * by changing `QUEUE_SIZE_MAX`.
 *
 * @date 09-07-2023
 *
 */

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <arduino-timer.h>
#include <string>

/* CDP Headers */
#include <PapaDuck.h>
#include <CdpPacket.h>
#include <queue>

#include "secrets.h"

#define CA_CERT
#ifdef CA_CERT
#endif

#define SSID "" // Your WiFi SSID (Make sure its a 2.4 Ghz network)
#define PASSWORD "" // Your WiFi Password

// Leave these empty
#define ORG         ""
#define DEVICE_ID   ""
#define DEVICE_TYPE ""
#define TOKEN       ""


char server[] = ORG ""; // Your AWS IoT Core Endpoint
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "TestPapa02";

#define CMD_STATE_WIFI "/wifi/"
#define CMD_STATE_HEALTH "/health/"
#define CMD_STATE_CHANNEL "/channel/"


// use the '+' wildcard so it subscribes to any command with any message format
const char commandTopic[] = "iot-2/cmd/+/fmt/+";

void gotMsg(char* topic, byte* payload, unsigned int payloadLength);

// Use pre-built papa duck
PapaDuck duck;

DuckDisplay* display = NULL;

bool use_auth_method = true;

auto timer = timer_create_default();

int QUEUE_SIZE_MAX = 5;
std::queue<std::vector<byte>> packetQueue;

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, gotMsg, wifiClient);

// / DMS locator URL requires a topicString, so we need to convert the topic
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
    case topics::bmp180:
      topicString = "bmp180";
      break;
    case topics::pir:
      topicString = "pir";
      break;
    case topics::dht11:
      topicString = "dht";
      break;
    case topics::bmp280:
      topicString = "bmp280";
      break;
    case topics::mq7:
      topicString = "mq7";
      break;
    case topics::gp2y:
      topicString = "gp2y";
      break;
    case reservedTopic::ack:
      topicString = "ack";
      break;
    default:
      topicString = "status";
  }

  return topicString;
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

// WiFi connection retry
bool retry = true;
int quackJson(CdpPacket packet) {

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

  Serial.println("[PAPA] Packet Received:");
  Serial.println("[PAPA] sduid:   " + String(sduid.c_str()));
  Serial.println("[PAPA] dduid:   " + String(dduid.c_str()));

  Serial.println("[PAPA] muid:    " + String(muid.c_str()));
  Serial.println("[PAPA] path:    " + String(path.c_str()));
  Serial.println("[PAPA] data:    " + String(payload.c_str()));
  Serial.println("[PAPA] hops:    " + String(packet.hopCount));
  Serial.println("[PAPA] duck:    " + String(packet.duckType));

  doc["DeviceID"] = sduid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["path"].set(path);
  doc["hops"].set(packet.hopCount);
  doc["duckType"].set(packet.duckType);

  std::string cdpTopic = toTopicString(packet.topic);

  display->clear();
  display->drawString(0, 10, "New Message");
  display->drawString(0, 20, sduid.c_str());
  display->drawString(0, 30, muid.c_str());
  display->drawString(0, 40, cdpTopic.c_str());
  display->sendBuffer();

  //  std::string topic = "iot-2/evt/" + cdpTopic + "/fmt/json";
  // std::string topic = "topic_2";
  std::string topic = "owl/device/" + sduid + "/evt/" + cdpTopic;

  String jsonstat;
  serializeJson(doc, jsonstat);

  //Filter out private chat so it won't get sent to DMS
  if(client.publish(topic.c_str(), jsonstat.c_str())) {
    Serial.println("[PAPA] Packet forwarded:");
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("[PAPA] Publish ok");
    display->drawString(0, 60, "Publish ok");
    display->sendBuffer();
    return 0;
  } else {
    Serial.println("[PAPA] Publish failed");
    display->drawString(0, 60, "Publish failed");
    display->sendBuffer();
    return -1;
  }
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(std::vector<byte> packetBuffer) {
  Serial.println("[PAPA] got packet: " +
                 convertToHex(packetBuffer.data(), packetBuffer.size()));

  CdpPacket packet = CdpPacket(packetBuffer);
  if(packet.topic != reservedTopic::ack) {
    if(quackJson(packet) == -1) {
      if(packetQueue.size() > QUEUE_SIZE_MAX) {
        packetQueue.pop();
        packetQueue.push(packetBuffer);
      } else {
        packetQueue.push(packetBuffer);
      }
      Serial.print("New size of queue: ");
      Serial.println(packetQueue.size());
    }
  }

  subscribeTo(commandTopic);
}

void setup() {
 // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("PAPADUCK");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  // the default setup is equivalent to the above setup sequence
  duck.setupWithDefaults(devId, SSID, PASSWORD);

  display = DuckDisplay::getInstance();
  // DuckDisplay instance is returned unconditionally, if there is no physical
  // display the functions will not do anything
  display->setupDisplay(duck.getType(), devId);

  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);

  #ifdef CA_CERT
  Serial.println("[PAPA] Using root CA cert");
  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);
  #else
  Serial.println("[PAPA] Using insecure TLS");
  wifiClient.setInsecure();
  #endif


  Serial.println("[PAPA] Setup OK! ");

   duck.enableAcks(true);
  // we are done
  display->showDefaultScreen();
}

void loop() {

   if (!duck.isWifiConnected() && retry) {
      String ssid = duck.getSsid();
      String password = duck.getPassword();

      Serial.println("[PAPA] WiFi disconnected, reconnecting to local network: " +
                     ssid);
                     
      int err = duck.reconnectWifi(ssid, password);

      if (err != DUCK_ERR_NONE) {
         retry = false;
      }
      timer.in(5000, enableRetry);
   }

   if (!client.loop()) {
     if(duck.isWifiConnected()) {
        mqttConnect();
     }

   }

   duck.run();
   timer.tick();

}

void gotMsg(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("gotMsg: invoked for topic: "); Serial.println(topic);

  if (String(topic).indexOf(CMD_STATE_WIFI) > 0) {
    Serial.println("Start WiFi Command");
    byte sCmd = 1;
    std::vector<byte> sValue = {payload[0]};

    if(payloadLength > 3) {
      std::string destination = "";
      for (int i=0; i<payloadLength; i++) {
        destination += (char)payload[i];
      }

      std::vector<byte> dDevId;
      dDevId.insert(dDevId.end(),destination.begin(),destination.end());

      duck.sendCommand(sCmd, sValue, dDevId);
    } else {
      duck.sendCommand(sCmd, sValue);
    }
  } else if (String(topic).indexOf(CMD_STATE_HEALTH) > 0) {
    byte sCmd = 0;
    std::vector<byte> sValue = {payload[0]};
    if(payloadLength >= 8) {
      std::string destination = "";
      for (int i=1; i<payloadLength; i++) {
        destination += (char)payload[i];
      }

      std::vector<byte> dDevId;
      dDevId.insert(dDevId.end(),destination.begin(),destination.end());

      duck.sendCommand(sCmd, sValue, dDevId);

    } else {
      Serial.println("Payload size too small");
    }
  } else {
    Serial.print("gotMsg: unexpected topic: "); Serial.println(topic);
  }
}

void wifiConnect() {
 Serial.print("Connecting to "); Serial.print(SSID);
 WiFi.begin(SSID, PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
 Serial.print("\nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void mqttConnect() {
   if (!!!client.connected()) {
      Serial.print("Reconnecting MQTT client to "); Serial.println(server);
      if(!!!client.connect(clientId) && retry) {
         Serial.print("Connection failed, retry in 5 seconds");
         retry = false;
         timer.in(5000, enableRetry);
      }
      Serial.println();
   } else {
      if(packetQueue.size() > 0) {
         publishQueue();
      }
      //Subscribe to command topic to receive commands from cloud
      subscribeTo(commandTopic);
   }

}

void subscribeTo(const char* topic) {
 Serial.print("subscribe to "); Serial.print(topic);
 if (client.subscribe(topic)) {
   Serial.println(" OK");
 } else {
   Serial.println(" FAILED");
 }
}

bool enableRetry(void*) {
  retry = true;
  return retry;
}

void publishQueue() {
  while(!packetQueue.empty()) {
    if(quackJson(packetQueue.front()) == 0) {
      packetQueue.pop();
      Serial.print("Queue size: ");
      Serial.println(packetQueue.size());
    } else {
      return;
    }
  }
}
