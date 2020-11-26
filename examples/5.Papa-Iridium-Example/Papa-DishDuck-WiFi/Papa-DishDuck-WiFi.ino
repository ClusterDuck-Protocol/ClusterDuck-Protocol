/**
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi and Sattelite enabled Papa Duck
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
PapaDuck duck = PapaDuck();

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

  // The Device ID must be unique and 8 bytes long. Typically the ID is stored
  // in a secure nvram, or provided to the duck during provisioning/registration
  std::vector<byte> devId = {'P', 'A', 'P', 'A', '0', '0', '0', '1'};

  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId, SSID, PASSWORD);
  setupRockBlock();

  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);

  Serial.println("[PAPA] Setup OK!");
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
    retry_mqtt_connection(WIFI_RETRY_DELAY_MS);
  }
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

void quackJson(std::vector<byte> packetBuffer) {

  CdpPacket packet = CdpPacket(packetBuffer);
  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  // Here we treat the internal payload of the CDP packet as a string
  // but this is mostly application dependent.
  // The parsingf here is optional. The Papa duck could simply decide to
  // forward the CDP packet as a byte array and let the Network Server (or DMS)
  // deal with the parsing based on some business logic.

  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  Serial.println("[PAPA] Packet Received:");
  Serial.println("[PAPA] sduid:   " + String(sduid.c_str()));
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
  std::string topic = "iot-2/evt/" + cdpTopic + "/fmt/json";

  String jsonstat;
  serializeJson(doc, jsonstat);

  if (client.publish(topic.c_str(), jsonstat.c_str())) {
    Serial.println("[PAPA] Packet forwarded:");
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("[PAPA] Publish ok");
  } else {
    Serial.println("[PAPA] Publish failed");
  }
}

void quackBeam(std::vector<byte> packetBuffer) {
  Serial.print("quackBeam");
  CdpPacket packet = CdpPacket(packetBuffer);
  int err;

  std::string payload(packet.data.begin(), packet.data.end());
  std::string duid(packet.sduid.begin(), packet.sduid.end());
  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  // Uncomment the lines below of which data you want to send
  String data = duid.c_str() + "/" + muid.c_str() + "/" + payload.c_str() +
                "/" + path.c_str();

  Serial.println(data);
  const char* text = data.c_str();
  // Send the message
  Serial.println(
      "Trying to send the message.  This might take several minutes.");
  err = modem.sendSBDText(text);
  if (err != ISBD_SUCCESS) {
    Serial.println("sendSBDText failed: error: ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Try again with a better view of the sky.");
  } else {
    Serial.println("Hey, it worked!");
  }
}

bool enableRetry(void*) {
  retry = true;
  return retry;
}

void retry_mqtt_connection(int delay_ms) {
  timer.tick();
  Serial.print(".");
  delay(delay_ms);
}

void setupRockBlock() {
  int signalQuality = -1;
  int err;

  // Start the serial port connected to the satellite modem
  Serial2.begin(19200, SERIAL_8N1, rxpin, txpin,
                false); // false means normal data polarity so steady state of
                        // line = 0, true means staedy state = high.
  // Begin satellite modem operation
  Serial.println("Starting modem...");
  err = modem.begin();
  if (err != ISBD_SUCCESS) {
    Serial.print("Modem begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
    return;
  }
  // Example: Print the firmware revision
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS) {
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
