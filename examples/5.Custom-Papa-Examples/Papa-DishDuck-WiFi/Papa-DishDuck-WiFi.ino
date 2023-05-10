/**
 * @file Papa-DishDuck-WiFi.ino
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi and Satellite enabled Papa Duck
 * 
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud.
 * 
 * @date 2020-11-10
 * ClusterDuck Protocol
 */

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <arduino-timer.h>
#include <string>

/* CDP Headers */
#include <CdpPacket.h>
#include <PapaDuck.h>

// Add Iridium Lib
#include <IridiumSBD.h>

//============== IRIDIUM ================
#define IridiumSerial Serial2
#define DIAGNOSTICS false // Change this to see diagnostics
#define rxpin 25
#define txpin 13

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);

#define MQTT_RETRY_DELAY_MS 500
#define WIFI_RETRY_DELAY_MS 5000

#define SSID ""
#define PASSWORD ""

// Used for Mqtt client connection
// Provided when a Papa Duck device is created in DMS
#define ORG ""
#define DEVICE_ID ""
#define DEVICE_TYPE ""
#define TOKEN ""

// Use pre-built papa duck
PapaDuck duck;

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

// Set this to false if testing quickstart on IBM IoT Platform
bool use_auth_method = true;

auto timer = timer_create_default(); // create a timer with default settings

WiFiClientSecure wifiClient;
PubSubClient client(server, 8883, wifiClient);

bool retry = true;

// This will be true when WiFi/MQTT is not available and instead the data is
// sent using Satellite connection
bool beamData = false;

void setup() {

  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("DISHDUCK");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId, SSID, PASSWORD);
  setupRockBlock();

  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);

  Serial.println("[DISH] Setup OK!");
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(std::vector<byte> packetBuffer) {
  if (beamData) {
    quackBeam(packetBuffer);
  } else {
    quackJson(packetBuffer);
  }
}

void loop() {

  if (!duck.isWifiConnected() && retry) {
    String ssid = duck.getSsid();
    String password = duck.getPassword();

    Serial.print("[DISH] WiFi disconnected, reconnecting to local network: ");
    Serial.print(ssid);

    int err = duck.reconnectWifi(ssid, password);

    if (err != DUCK_ERR_NONE) {
      retry = false;
      timer.in(WIFI_RETRY_DELAY_MS, enableRetry);
    }
  }
  if (duck.isWifiConnected()) {
    setup_mqtt(use_auth_method);
    beamData = false;
  } else {
    beamData = true;
  }

  duck.run();
  timer.tick();
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

void quackJson(std::vector<byte> packetBuffer) {

  CdpPacket packet = CdpPacket(packetBuffer);
  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  // Here we treat the internal payload of the CDP packet as a string
  // but this is mostly application dependent.
  // The parsing here is optional. The Papa duck could simply decide to
  // forward the CDP packet as a byte array and let the Network Server (or DMS)
  // deal with the parsing based on some business logic.

  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string dduid(packet.dduid.begin(), packet.dduid.end());

  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  Serial.println("[DISH] Packet Received:");
  Serial.println("[DISH] sduid:   " + String(sduid.c_str()));
  Serial.println("[DISH] dduid:   " + String(dduid.c_str()));

  Serial.println("[DISH] muid:    " + String(muid.c_str()));
  Serial.println("[DISH] path:    " + String(path.c_str()));
  Serial.println("[DISH] data:    " + String(payload.c_str()));
  Serial.println("[DISH] hops:    " + String(packet.hopCount));
  Serial.println("[DISH] duck:    " + String(packet.duckType));

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

  //Filter out private chat so it won't get sent to DMS
  if(cdpTopic == "pchat") {
    return -1;
  } else if(client.publish(topic.c_str(), jsonstat.c_str())) {
    Serial.println("[DISH] Packet forwarded:");
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("[DISH] Publish ok");
  } else {
    Serial.println("[DISH] Publish failed");
  }
}

void quackBeam(std::vector<byte> packetBuffer) {
  Serial.print("[DISH] quackBeam");
  int err;

  CdpPacket packet = CdpPacket(packetBuffer);
  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  // Here we treat the internal payload of the CDP packet as a string
  // but this is mostly application dependent. 
  // The parsing here is optional. The Papa duck could simply decide to
  // forward the CDP packet as a byte array and let the Network Server (or DMS) deal with
  // the parsing based on some business logic.

  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string dduid(packet.dduid.begin(), packet.dduid.end());

  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  Serial.println("[DISH] Packet Received:");
  Serial.println("[DISH] sduid:   " + String(sduid.c_str()));
  Serial.println("[DISH] dduid:   " + String(dduid.c_str()));

  Serial.println("[DISH] muid:    " + String(muid.c_str()));
  Serial.println("[DISH] path:    " + String(path.c_str()));
  Serial.println("[DISH] data:    " + String(payload.c_str()));
  Serial.println("[DISH] hops:    " + String(packet.hopCount));
  Serial.println("[DISH] duck:    " + String(packet.duckType));

  std::string cdpTopic = toTopicString(packet.topic);

  String data = String(sduid.c_str()) + "/" + String(muid.c_str()) + "/" + String(payload.c_str()) +
                "/" + String(path.c_str()) + "/" + String(cdpTopic.c_str());

                Serial.println(data);
  const   char *text = data.c_str();
  // Send the message
  Serial.print("[DISH] Trying to send the message.  This might take several minutes.\r\n");
  err = modem.sendSBDText(text);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("[DISH] sendSBDText failed: error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("[DISH] Try again with a better view of the sky.");
  }
  else
  {
    Serial.println("[DISH] Message Sent to Iridium!");
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

void setupRockBlock() {
  int signalQuality = -1;
  int err;

  // Start the serial port connected to the satellite modem
  Serial2.begin(19200, SERIAL_8N1, rxpin, txpin,
                false); // false means normal data polarity so steady state of
                        // line = 0, true means staedy state = high.
  // Begin satellite modem operation
  Serial.println("[DISH] Starting modem...");
  err = modem.begin();
  if (err != ISBD_SUCCESS) {
    Serial.print("[DISH] Modem begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("[DISH] No modem detected: check wiring.");
    return;
  }
  // Example: Print the firmware revision
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS) {
    Serial.print("[DISH] FirmwareVersion failed: error ");
    Serial.println(err);
    return;
  }

  {
    Serial.print("[DISH] Firmware Version is ");
    Serial.print(version);
    Serial.println(".");
  }
}
