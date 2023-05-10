/**
 * @file Papa-Downtime-Counter.ino
 * @author 
 * @brief Uses pre-built PapaDuck and will send downtime data after reconnecting
 * 
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud. It will also send
 * how long it was disconnected from cloud network as well as how many times
 * it has disconnected. Watch working session
 * https://www.youtube.com/watch?v=xx4aYct8m7o&t
 * 
 * @date 2021-04-08
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
PapaDuck duck;

DuckDisplay* display = NULL;

// Set this to false if testing quickstart on IBM IoT Platform
bool use_auth_method = true;

auto timer = timer_create_default();

int QUEUE_SIZE_MAX = 5;
std::queue<std::vector<byte>> packetQueue;

int disconnectTime;
bool disconnect = false;
int failCounter = 0;

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
    case topics::bmp180:
      topicString ="bmp";
      break;
    case topics::pir:
      topicString ="pir";
      break;
    case topics::dht11:
      topicString ="dht";
      break;
    case topics::bmp280:
      topicString ="bmp280";
      break;
    case topics::mq7:
      topicString ="mq7";
      break;
    case topics::gp2y:
      topicString ="gp2y";
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
int quackJson(std::vector<byte> packetBuffer) {

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
  
//Filter out private chat so it won't get sent to DMS
  if(cdpTopic == "pchat") {
    return -1;
  } else if(client.publish(topic.c_str(), jsonstat.c_str())) {
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
    failCounter++;
    return -1;
  }
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(std::vector<byte> packetBuffer) {
  Serial.println("[PAPA] got packet: " +
                 convertToHex(packetBuffer.data(), packetBuffer.size()));
  if(quackJson(packetBuffer) == -1) {
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
  
  //Send report on how many disconnects in a period
  timer.every(3000000, sendFailReport);

  Serial.println("[PAPA] Setup OK! ");
  
  // we are done
  display->showDefaultScreen();
}



void loop() {

  if (!duck.isWifiConnected() && retry) {
    if(!disconnect) {
      disconnectTime = millis();
      disconnect = true;
    }

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
    //Once reconnected check queue and publish all queued messages
    if(packetQueue.size() > 0) {
      publishQueue();
    }
    return;
  }

  if(!disconnect) {
    disconnectTime = millis();
    disconnect = true;
  }

  if (use_auth) {
    connected = client.connect(clientId, authMethod, token);
  } else {
    connected = client.connect(clientId);
  }
  if (connected) {
    Serial.println("[PAPA] Mqtt client is connected!");
    disconnect = false;
    int timeDisconnected = millis() - disconnectTime;
    timeDisconnected = timeDisconnected/1000; //Time in seconds

    //It is normal to not have a continous connection to MQTT and disconnect for a short period
    if(timeDisconnected > 1) quackDownReport("Time Disconnected: " + String(timeDisconnected));
    if(packetQueue.size() > 0) {
      publishQueue();
    }
    return;
  }
  retry_mqtt_connection(1000);
  
}

void quackDownReport(String payload) {
  Serial.println("Send QuackDownReport");

  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  doc["DeviceID"] = "PAPADUCK";
  doc["MessageID"] = createUuid(4);
  doc["Payload"].set(payload);
  doc["path"] = "PAPADUCK";
  doc["hops"] = "0";
  doc["duckType"] = "1";
    
  std::string topic = "iot-2/evt/papahealth/fmt/json";

  String jsonstat;
  serializeJson(doc, jsonstat);

  //Filter out private chat so it won't get sent to DMS
  if(cdpTopic == "pchat") {
    return -1;
  } else if(client.publish(topic.c_str(), jsonstat.c_str())) {
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
    failCounter++;
  }

}

// Creates a unique id for the message
String createUuid(int length) {
  String msg = "";
  int i;

  for (i = 0; i < length; i++) {
    byte randomValue = random(0, 36);
    if (randomValue < 26) {
      msg = msg + char(randomValue + 'a');
    } else {
      msg = msg + char((randomValue - 26) + '0');
    }
  }
  return msg;
}

bool sendFailReport(void*) {
  quackDownReport("Packet Fail count: " + String(failCounter));
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

bool enableRetry(void*) {
  retry = true;
  return retry;
}

void retry_mqtt_connection(int delay_ms) {
  Serial.println("[PAPA] Could not connect to MQTT...............................");
  retry = false;
  timer.in(delay_ms, enableRetry);
}
