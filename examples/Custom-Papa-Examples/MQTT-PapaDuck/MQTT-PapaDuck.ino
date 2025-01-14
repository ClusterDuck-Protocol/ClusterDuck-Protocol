
/**
 * @file PapaDuck-Mqtt.ino
 * @author
 * @brief This example creates a PapaDuck that receives messages from other ducks and sends them to the test Mosquitto MQTT broker.
 *
 * It's using a pre-built PapaDuck available from the ClusterDuck SDK but configured to send messages to an MQTT broker.
 */

#include <CDP.h>

#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>
#include <arduino-timer.h>
#include <string>

#include <queue>


#define MQTT_RETRY_DELAY_MS 500
#define WIFI_RETRY_DELAY_MS 5000

#define MQTT_SERVER     "test.mosquitto.org" 
#define PORT            8883
#define MQTT_CLIENT_ID  "papa-duck-mqtt-1"  // This must be unique


// from https://test.mosquitto.org/
static const char* mosquitto_ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n" \
"BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\n" \
"A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\n" \
"BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\n" \
"by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\n" \
"BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\n" \
"MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\n" \
"dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\n" \
"KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\n" \
"UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\n" \
"Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\n" \
"s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\n" \
"3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\n" \
"E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\n" \
"MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\n" \
"6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n" \
"BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\n" \
"6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\n" \
"+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\n" \
"sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\n" \
"LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\n" \
"m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n" \
"-----END CERTIFICATE-----\n";


// Use pre-built papa duck as our hub device
PapaDuck hub;

bool setupOK = false;

auto timer = timer_create_default();
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
bool wifiConnected = false;

// hub incoming mqtt messages
std::queue<std::string> mqttMessageQueue;

std::string mqttPubTopic = "hub/event";           // published by the hub
std::string mqttSubTopic = "incoming/say_hello";  // subscribed by the hub

const std::string WIFI_SSID="ENTER SSID";   // Replace with WiFi SSID
const std::string WIFI_PASS="ENTER PASSWORD";     // Replace with WiFi Password

std::string toTopicString(byte topic);
std::string arduinoStringFromHex(byte* data, int size);
bool setup_mqtt(void);
void handleIncomingMqttMessages(void);
// std::string toHexString(std::vector<byte>& data);
std::vector<uint8_t> fromHexString(const std::string& hexString);

// std::string toHexString(std::vector<byte>& data) {
//   std::string str = arduinoStringFromHex(data.data(), data.size());
//   return str.c_str();
// }

std::vector<uint8_t> fromHexString(const std::string& hexString) {
    if (hexString.length() % 2 != 0) {
        throw std::invalid_argument("Hex string must have an even length");
    }

    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hexString.length(); i += 2) {
        std::string byteString = hexString.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

bool setup_mqtt(void) 
{
    bool connected = mqttClient.connected();
    if (connected) {
        return false;
    }
    
    Serial.println("[HUB] MQTT client connecting to broker...");

    // Connect to the MQTT broker with the client ID only
    // If you need to use a username and password, use the connect method with 3 parameters below
    // boolean mqttClient.connect(const char *id, const char *user, const char *pass) 
    connected = mqttClient.connect(MQTT_CLIENT_ID);
    if (!connected) {
        Serial.println("[HUB] ERROR - Failed to connect to MQTT broker");
        return false;
    }

    Serial.println("[HUB] MQTT client connected");

    // This is an example if you want to subscribe to an incoming topic
    if (!mqttClient.subscribe(mqttSubTopic.c_str(),0)) {
        Serial.println("[HUB] ERROR - Failed to subscribe to topic");
        return false;
    }
    
    return true;
}

std::string toTopicString(byte topic) 
{

  std::string topicString;

  switch (topic) {
    case topics::status:
      topicString = "status";
      break;
    case topics::sensor:
      topicString = "sensor";
      break;
    default:
      topicString = "status";
  }
  return topicString;
}

std::string arduinoStringFromHex(byte* data, int size) 
{
  std::string buf = "";
  buf.reserve(size * 2); // 2 digit hex
  const char* cs = "0123456789ABCDEF";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0F];
    buf += cs[val & 0x0F];
  }
  return buf;
}

bool publishToMqttTopic(std::string source, std::string topic, std::string message) {
  bool ok = false;
  if (hub.isWifiConnected()) {
    ok = setup_mqtt();  
    if (!ok) {
      Serial.println("[HUB] ERROR - Failed to connect to MQTT broker");
      return false;
    }
    if (mqttClient.publish(topic.c_str(), message.c_str(), message.length())) {
      ok = true;
      Serial.println("[HUB] Publish ok");        
    } else {
      ok = false;
      Serial.println("[HUB] Publish failed");
    }
  } else {
    Serial.println("[HUB] ERROR No WiFi connection!!!");
    ok = false;
  }  
  return ok;
}


// Incoming MQTT messages from the controller
// This needs to be fast, so we simply queue the raw message
// And we can process them in the main loop
void mqtt_callback(char* topic, byte* message, unsigned int length) {
  
  // Convert byte array to std::string
  std::string msg(message, message + length);

  Serial.printf("[HUB] Message arrived on topic: %s\n", topic);
  Serial.printf("[HUB] queuing msg: %s\n", msg.c_str());
 
  // Push the raw message
  mqttMessageQueue.push(msg);
}

void handleIncomingMqttMessages(void) {

  while (!mqttMessageQueue.empty()) {
    auto rawMessage = mqttMessageQueue.front(); 
    mqttMessageQueue.pop();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, rawMessage);
    if (err) {
      Serial.print("[HUB] deserializeJson() failed with code: ");
      Serial.println(err.c_str());
      continue;
    }
    std::string jsonstat;
    serializeJson(doc, jsonstat);
    serializeJsonPretty(doc, Serial);

    // Process the topic and message here
  }
}


void processMessageFromDucks(std::vector<byte> packetBuffer) {

    JsonDocument doc;
    CdpPacket cdp_packet = CdpPacket(packetBuffer);

    int messageLength = cdp_packet.data.size();

    Serial.printf("Packet data size=%d\n", messageLength);

    std::string muid(cdp_packet.muid.begin(), cdp_packet.muid.end());
    std::string sduid(cdp_packet.sduid.begin(), cdp_packet.sduid.end());
    std::string cdpTopic = toTopicString(cdp_packet.topic);

    Serial.printf("[HUB] got topic: %s from %s\n",cdpTopic.c_str(), sduid.c_str());
 
    // Counter Message
    std::string payload(cdp_packet.data.begin(), cdp_packet.data.end());

    // Forward the counter message to the MQTT broker
    // This is a simple example, but you can do anything you want with the message here
    // This example shows how the message from be transformed into something that matches your application
    // uint32_t msgId = esp_random();
    doc["from"] = "hub";
    doc["to"] = "controller";
    doc["RE"] = false; // This flag is used to indicate if the controller should respond to this message
    doc["eventType"] = cdpTopic.c_str();
    doc["MessageID"].set(muid);
    
    doc["payload"]["hops"].set(cdp_packet.hopCount);
    doc["payload"]["duckType"].set(cdp_packet.duckType);
    doc["payload"]["DeviceID"] = sduid.c_str();

    doc["payload"]["Message"] = payload.c_str();  
       
    std::string jsonstat;
    serializeJson(doc, jsonstat);
    Serial.printf("%s\n",jsonstat.c_str());

    if (hub.isWifiConnected()) {
      setup_mqtt();  
      if (mqttClient.publish(mqttPubTopic.c_str(), jsonstat.c_str(), jsonstat.length())) {
        Serial.println("[HUB] Packet forwarded:");
        serializeJsonPretty(doc, Serial);
        Serial.println("");
        Serial.println("[HUB] Publish ok");
        
      } else {
        Serial.println("[HUB] Publish failed");
      }

    } else {
      Serial.println("[HUB] ERROR No WiFi connection!!!");
    }
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over MQTT
void handleDuckData(std::vector<byte> packetBuffer) {
  Serial.printf("[HUB] got packet: %s\n", arduinoStringFromHex(packetBuffer.data(), packetBuffer.size()).c_str());
  processMessageFromDucks(packetBuffer);
}


void setup() {
  std::string deviceId("PAPA0001");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  
  // Set the CA cert for the WiFi client
  wifiClient.setCACert(mosquitto_ca_cert);

  // Setup the duck link with default settings and connect to WiFi
  uint32_t err = hub.setupWithDefaults(devId, WIFI_SSID, WIFI_PASS);

  // If we fail to connect to WiFi, retry a few times
  if (err == DUCK_INTERNET_ERR_CONNECT) {
    int retry=0;
    while ( err ==  DUCK_INTERNET_ERR_CONNECT && retry < 3 ) {
      Serial.printf("[HUB] WiFi disconnected, retry connection: %s\n",WIFI_SSID.c_str());
      delay(5000);
      err = hub.setupInternet(WIFI_SSID, WIFI_PASS);
      retry++;
    }  
  }
  if (err != DUCK_ERR_NONE) {
    Serial.print("[HUB] Failed to setup PapaDuck: rc = ");Serial.println(err);
    setupOK = false;
    return;
  }

  setupOK = true;
  // register a callback to handle incoming data from duck in the network
  hub.onReceiveDuckData(handleDuckData);

  // setup MQTT client
  mqttClient.setServer(MQTT_SERVER, PORT);
  mqttClient.setCallback(mqtt_callback);
  mqttClient.setKeepAlive(60);
  
  if (!setup_mqtt()) {
    setupOK = false;
    return;
  }
   
  while (!hub.isWifiConnected()) {
    delay(1000);
  }
  
  Serial.printf("[HUB] Ready!\n");
}

void loop() 
{
  if (!setupOK) {
    return;
  }
  hub.run();
  timer.tick();

  // Check for incoming messages from MQTT
  mqttClient.loop();
  handleIncomingMqttMessages();
}
