/**
 * @file PapaDuck.ino
 * @author Timo Wielink
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 *
 * This example will configure and run a Papa Duck that connects to AWS cloud
 * and forwards all messages (except pings) to the cloud. When disconnected
 * it will add received packets to a queue. When it reconnects to MQTT it will
 * try to publish all messages in the queue. You can change the size of the queue
 * by changing `QUEUE_SIZE_MAX`.
 *
 * @date 05-07-2025
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

// --- WiFi Configuration ---
#define SSID ""                    // Your WiFi SSID (Needs to be 2.4 Ghz network)
#define PASSWORD ""                // Your WiFi Password

// --- Command Definitions ---
#define CMD_STATE_WIFI "/wifi/"
#define CMD_STATE_HEALTH "/health/"
#define CMD_STATE_CHANNEL "/channel/"

// --- Global Objects ---
PapaDuck duck("PAPADUCK");
int QUEUE_SIZE_MAX = 5;
auto timer = timer_create_default();
bool retry = true; 
const char commandTopic[] = "iot-2/cmd/+/fmt/+";

// --- Function Declarations ---
std::queue<std::vector<byte>> packetQueue;
std::string toTopicString(byte topic);
std::string convertToHex(byte* data, int size);
int quackJson(CdpPacket packet);
void handleDuckData(std::vector<byte> packetBuffer);
void gotMsg(char* topic, byte* payload, unsigned int payloadLength);
void wifiConnect();
void mqttConnect();
void subscribeTo(const char* topic);
bool enableRetry(void*);
void publishQueue();

// --- WIFI Setup Function ---
WiFiClientSecure wifiClient;
PubSubClient client(AWS_IOT_ENDPOINT, 8883, gotMsg, wifiClient);

/**
 * @brief Converts a CDP topic byte value into a string.
 *
 * This utility function maps a given `byte` topic identifier to its corresponding
 * topic name string. If the topic does not match any known value, it defaults to 
 * `"status"`.
 *
 * @param topic The CDP topic identifier (byte) from a received packet
 * @return A std::string representing the corresponding topic name
 */
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

/**
 * @brief Converts a byte buffer into a hexadecimal string representation.
 *
 * This utility function takes a raw byte array and converts it into a readable
 * hexadecimal string. Each byte is represented by two hexadecimal characters. This 
 * is useful for debugging or displaying binary data in logs or user interfaces.
 *
 * @param data Pointer to the byte array to convert
 * @param size Number of bytes to convert
 * @return std::string A string containing the hexadecimal representation of the byte array
 */
std::string convertToHex(byte* data, int size) { //is this in duckutils?
  std::string buf = "";
  buf.reserve(size * 2);
  const char* cs = "0123456789ABCDEF";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0F];
    buf += cs[val & 0x0F];
  }
  return buf;
}

/**
 * @brief Converts a received CDP packet into JSON format and publishes it to an MQTT topic.
 * 
 * This function parses key metadata fields from the incoming `CdpPacket`. These fields are 
 * encoded into a structured JSON document and then serialized into a string. The JSON 
 * message is published to an MQTT topic derived from the packet topic code using the 
 * `toTopicString` function. The topic format is: `owl/device/THINGNAME/evt/<topic>`
 *
 * @param packet A CDP packet received from the mesh network
 * @return int Returns 0 if the message is successfully published to MQTT; -1 on failure
 */
int quackJson(CdpPacket packet) {

  JsonDocument doc;

  // Parsing the packet that was received
  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string dduid(packet.dduid.begin(), packet.dduid.end());
  std::string muid(packet.muid.begin(), packet.muid.end());

  Serial.println("[PAPA] Packet Received:");
  Serial.printf("[PAPA] sduid:   %s\n" , sduid.c_str());
  Serial.printf("[PAPA] dduid:   %s\n" , dduid.c_str());
  Serial.printf("[PAPA] muid:    %s\n" , muid.c_str());
  Serial.printf("[PAPA] data:    %s\n" , payload.c_str());
  Serial.printf("[PAPA] hops:    %s\n", std::to_string(packet.hopCount).c_str());
  Serial.printf("[PAPA] duck:    %s\n" , std::to_string(packet.duckType).c_str());

  doc["DeviceID"] = sduid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["hops"].set(packet.hopCount);
  doc["duckType"].set(packet.duckType);

  std::string cdpTopic = packet.toTopicString();

  std::string topic = "owl/device/" + std::string(THINGNAME) + "/evt/" + cdpTopic;

  std::string jsonstat;
  serializeJson(doc, jsonstat);

  if(client.publish(topic.c_str(), jsonstat.c_str())) {
    Serial.println("[PAPA] Packet forwarded:");
    serializeJsonPretty(doc, Serial);
    Serial.println("");
    Serial.println("[PAPA] Publish ok");
    return 0;
  } else {
    Serial.println("[PAPA] Publish failed");
    return -1;
  }
}

/**
 * @brief Callback function to handle incoming data from the Papa Duck.
 *
 * This function is invoked when a packet is received. It processes the packet,
 * checks if it is an ACK, and if not, adds it to a queue for later processing.
 * The function also subscribes to the command topic for further commands.
 *
 * @param packetBuffer The received packet buffer
 */
void handleDuckData(std::vector<byte> packetBuffer) {
  Serial.printf("[PAPA] got packet: %s\n" ,
                 convertToHex(packetBuffer.data(), packetBuffer.size()).c_str());

  CdpPacket packet = CdpPacket(packetBuffer);
  if(packet.topic != reservedTopic::ack) {
    if(quackJson(packet) == -1) {
      if(packetQueue.size() > QUEUE_SIZE_MAX) {
        packetQueue.pop();
        packetQueue.push(packetBuffer);
      } else {
        packetQueue.push(packetBuffer);
      }
      Serial.print("[PAPA] New size of queue: ");
      Serial.println(packetQueue.size());
    }
  }

  subscribeTo(commandTopic);
}

/**
 * @brief Initializes the PapaDuck node with device identity, network configuration, and MQTT security credentials.
 * 
 * - Sets the unique 8-byte Duck ID for the PapaDuck.
 * - Initializes the PapaDuck firmware with default radio and network settings.
 * - Registers a callback (`handleDuckData`) to process incoming CDP packets.
 * - Configures TLS security for MQTT communication:
 * 
 * This function should be called once at boot to prepare the PapaDuck for participation in 
 * the ClusterDuck mesh and MQTT forwarding system.
 */
void setup() {
  duck.setupWithDefaults();
  duck.joinWifiNetwork(SSID, PASSWORD);
  
  duck.onReceiveDuckData(handleDuckData);     // Callback handling incoming data from the network

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
}

void loop() {
  
  if (!duck.isWifiConnected() && retry) {
    std::string ssid = duck.getSsid();
    std::string password = duck.getPassword();

    Serial.println(("[PAPA] WiFi disconnected, reconnecting to local network: " + ssid).c_str());
                     
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
  Serial.print("[PAPA] gotMsg: invoked for topic: "); Serial.println(topic);

  if (std::string(topic).find(CMD_STATE_WIFI) > 0) {
    Serial.println("[PAPA] Start WiFi Command");
    byte sCmd = 1;
    std::vector<byte> sValue = {payload[0]};

    if(payloadLength > 3) {
      std::string destination = "";
      for (int i=0; i<payloadLength; i++) {
        destination += (char)payload[i];
      }

        std::array<byte,8> dDevId;
        std::copy(destination.begin(), destination.end(), dDevId.begin());

      duck.sendCommand(sCmd, sValue, dDevId);
    } else {
      duck.sendCommand(sCmd, sValue);
    }
  } else if (std::string(topic).find(CMD_STATE_HEALTH) > 0) {
    byte sCmd = 0;
    std::vector<byte> sValue = {payload[0]};
    if(payloadLength >= 8) {
      std::string destination = "";
      for (int i=1; i<payloadLength; i++) {
        destination += (char)payload[i];
      }

      std::array<byte,8> dDevId;
      std::copy(destination.begin(), destination.end(), dDevId.begin());

      duck.sendCommand(sCmd, sValue, dDevId);

    } else {
      Serial.println("[PAPA] Payload size too small");
    }
  } else {
    Serial.print("[PAPA] gotMsg: unexpected topic: "); Serial.println(topic);
  }
}

void wifiConnect() {
 Serial.print("[PAPA] Connecting to "); Serial.print(SSID);
 WiFi.begin(SSID, PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
 Serial.print("\n[PAPA] WiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void mqttConnect() {
   if (!!!client.connected()) {
      Serial.print("[PAPA] Reconnecting MQTT client to "); Serial.println(AWS_IOT_ENDPOINT);
      if(!!!client.connect(THINGNAME) && retry) {
         Serial.print("[PAPA] Connection failed, retry in 5 seconds");
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
 Serial.print("[PAPA] subscribe to "); Serial.print(topic);
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
      Serial.print("[PAPA] Queue size: ");
      Serial.println(packetQueue.size());
    } else {
      return;
    }
  }
}