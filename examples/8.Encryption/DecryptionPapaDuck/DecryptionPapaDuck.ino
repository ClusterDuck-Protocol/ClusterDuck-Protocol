/**
 * @file Decryption PapaDuck example
 * @author 
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck and decrypts messages from network
 * 
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud.
 * 
 * @date 2021-2-25
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

#define MQTT_RETRY_DELAY_MS 500
#define WIFI_RETRY_DELAY_MS 5000

#define SSID ""
#define PASSWORD ""




// Used for Mqtt client connection
// Provided when a Papa Duck device is created in DMS
#define ORG         ""
#define DEVICE_ID   ""
#define DEVICE_TYPE ""
#define TOKEN       ""
char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

// Use pre-built papa duck
PapaDuck duck = PapaDuck();

DuckDisplay* display = NULL;

// Set this to false if testing quickstart on IBM IoT Platform
bool use_auth_method = true;

auto timer = timer_create_default();

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);
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
      topicString ="health";
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
void quackJson(std::vector<byte> packetBuffer) {

   CdpPacket packet = CdpPacket(packetBuffer);
   const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
   StaticJsonDocument<bufferSize> doc;

   // Here we treat the internal payload of the CDP packet as a string
   // but this is mostly application dependent. 
   // The parsingf here is optional. The Papa duck could simply decide to
   // forward the CDP packet as a byte array and let the Network Server (or DMS) deal with
   // the parsing based on some business logic.

   std::vector<uint8_t> encrypted;
   encrypted.insert(encrypted.end(), packet.data.begin(), packet.data.end());
   duck.decrypt(encrypted.data(), packet.data.data(), encrypted.size());

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
      
   std::string topic = "iot-2/evt/" + cdpTopic + "/fmt/json";

   String jsonstat;
   serializeJson(doc, jsonstat);

   if (client.publish(topic.c_str(), jsonstat.c_str())) {
      Serial.println("[PAPA] Packet forwarded:");
      serializeJsonPretty(doc, Serial);
      Serial.println("");
      Serial.println("[PAPA] Publish ok");
      display->drawString(0, 60, "Publish ok");
      display->sendBuffer();
   } else {
      Serial.println("[PAPA] Publish failed");
      display->drawString(0, 60, "Publish failed");
      display->sendBuffer();
   }
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(std::vector<byte> packetBuffer) {
  Serial.println("[PAPA] got packet: " +
                 convertToHex(packetBuffer.data(), packetBuffer.size()));
  quackJson(packetBuffer);
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

  Serial.println("[PAPA] Setup OK! ");
  
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
      timer.in(5000, enableRetry);
    }
  }
  if (duck.isWifiConnected() && retry) {
    setup_mqtt(use_auth_method);
  }

  duck.run();
  timer.tick();
}

void setup_mqtt(bool use_auth) {
  bool connected = client.connected();
  if (connected) {
    return;
  }

  
  if (use_auth) {
    connected = client.connect(clientId, authMethod, token);
  } else {
    connected = client.connect(clientId);
  }
  if (connected) {
    Serial.println("[PAPA] Mqtt client is connected!");
    return;
  }
  retry_mqtt_connection(1000);
  
}





bool enableRetry(void*) {
  retry = true;
  return retry;
}

void retry_mqtt_connection(int delay_ms) {
  Serial.println("[PAPA] Could not connect to MQTT...............................");
  retry = false;
  timer.in(delay_ms, enableRetry);
}