/**
 * @file papa-duck-with-callback.ino
 * @author 
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 * 
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud.
 * 
 * @date 2020-09-21
 * 
 */
 
#include<string>

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <arduino-timer.h>

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



// Use pre-built papa duck
PapaDuck duck = PapaDuck();

char server[]           = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[]       = "use-token-auth";
char token[]            = TOKEN;
char clientId[]         = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

// Set this to false if testing quickstart on IBM IoT Platform
bool use_auth_method = true;

auto timer = timer_create_default(); // create a timer with default settings

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);

bool retry = true;

void setup() {
  
  // We are using a hardcoded device id here, but it should be retrieved or given during the device provisioning
  // then converted to a byte vector to setup the duck
  // NOTE: The Device ID must be exactly 8 bytes otherwise it will get rejected
  std::string deviceId(DEVICE_ID);
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  
  // Use the default setup provided by the SDK  
  duck.setupWithDefaults(devId, SSID, PASSWORD);

  
  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);
  
  Serial.println("[PAPA] Setup OK!");
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(CDP_Packet packet) {
  if (packet.topic == reservedTopic::ping) {
    return;
  }
  
  quackJson(packet);
}


void loop() {

  if (!duck.isWifiConnected() && retry) {
    String ssid = duck.getSsid();
    String password = duck.getPassword();
    
    Serial.print("[PAPA] WiFi disconnected, reconnecting to local network: ");
    Serial.print(ssid);

    int err = duck.reconnectWifi(ssid, password);

    if (err != DUCK_ERR_NONE) {
      retry = false;
      timer.in(WIFI_RETRY_DELAY_MS, enableRetry);
    }
  }
  if (duck.isWifiConnected()) {
    setup_mqtt(use_auth_method);
  }

  duck.run();
  timer.tick();
}

void setup_mqtt(bool use_auth)
{
  bool connected = client.connected();
  if (connected) {
    return;
  }
  
  for (;;) {
    if (use_auth) {
      connected = client.connect(clientId, authMethod, token);
    } else {
      connected = client.connect(clientId);
    }
    if (connected) {
      Serial.println("[PAPA] mqtt client is connected!");
      break;
    }
    retry_mqtt_connection(MQTT_RETRY_DELAY_MS);
  }
}

// DMS locator URL requires a topicString, so we need to convert the topic
// from the packet to a string based on the topics code
std::string toTopicString(byte topic) {
  
  std::string topicString;
  
  switch (topic) {
    case  topics::status:
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

void quackJson(CDP_Packet packet) {
  
  const int bufferSize = 4 *  JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  std::string payload(packet.data.begin(),packet.data.end());
  std::string duid(packet.duid.begin(),packet.duid.end());
  std::string muid(packet.muid.begin(),packet.muid.end());
  std::string path(packet.path.begin(),packet.path.end());

  doc["DeviceID"]  = duid;
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
  }
  else {
    Serial.println("Publish failed");
  }
}

bool enableRetry(void *) {
  retry = true;
  return retry;
}

void retry_mqtt_connection(int delay_ms) {
  timer.tick();
  Serial.print(".");
  delay(delay_ms);
}
