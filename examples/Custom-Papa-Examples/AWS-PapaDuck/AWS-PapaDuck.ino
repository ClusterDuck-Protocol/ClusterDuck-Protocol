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
#include <CDP.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
// #include <arduino-timer.h>
#include <queue>
#include "secrets.h"

#define CA_CERT
#ifdef CA_CERT
#endif

// --- WiFi Configuration ---
#define WIFI_SSID ""                    // Your WiFi SSID (Needs to be 2.4 Ghz network)
#define WIFI_PASSWORD ""                // Your WiFi Password

// --- Command Definitions ---
#define CMD_STATE_WIFI "/wifi/"
#define CMD_STATE_HEALTH "/health/"
#define CMD_STATE_CHANNEL "/channel/"

// Setup for W2812 (LED)
#include <FastLED.h>
#include <pixeltypes.h>
#define LED_TYPE WS2812
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
CRGB leds[NUM_LEDS];

// --- Global Objects ---
PapaDuck duck(THINGNAME);
int QUEUE_SIZE_MAX = 5;
auto timer = timer_create_default();
bool retry = true; 

// --- Function Declarations ---
std::queue<std::vector<byte>> packetQueue;
int quackJson(CdpPacket packet);
void handleDuckData(CdpPacket receivedPacket);
void mqttConnect();
void subscribeTo(const char* topic);
bool enableRetry(void*);
void publishQueue();

// --- WIFI Setup Function ---
WiFiClientSecure wifiClient;
PubSubClient client(AWS_IOT_ENDPOINT, 8883, wifiClient);

/**
 * @brief Converts a received CDP packet into JSON format and publishes it to an MQTT topic.
 * 
 * This function parses key metadata fields from the incoming `CdpPacket`. These fields are 
 * encoded into a structured JSON document and then serialized into a string. The JSON 
 * message is published to an MQTT topic derived from the packet topic code using the 
 * `topicToString` function. The topic format is: `owl/device/THINGNAME/evt/<topic>`
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

  loginfo_ln("[PAPA] Packet Received:");
  loginfo("[PAPA] sduid:   %s\n" , sduid.c_str());
  loginfo("[PAPA] dduid:   %s\n" , dduid.c_str());
  loginfo("[PAPA] muid:    %s\n" , muid.c_str());
  loginfo("[PAPA] data:    %s\n" , payload.c_str());
  loginfo("[PAPA] hops:    %s\n", std::to_string(packet.hopCount).c_str());
  loginfo("[PAPA] duck:    %s\n" , std::to_string(packet.duckType).c_str());

  doc["DeviceID"] = sduid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["hops"].set(packet.hopCount);
  doc["duckType"].set(packet.duckType);

  std::string cdpTopic = packet.topicToString();

  std::string topic = "owl/device/" + std::string(THINGNAME) + "/evt/" + cdpTopic;

  std::string jsonstat;
  serializeJson(doc, jsonstat);

  if(client.publish(topic.c_str(), jsonstat.c_str())) {
    loginfo_ln("[PAPA] Packet forwarded:");
    serializeJsonPretty(doc, Serial);
    loginfo_ln("");
    loginfo_ln("[PAPA] Publish ok");
    return 0;
  } else {
    loginfo_ln("[PAPA] Publish failed");
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
void handleDuckData(CdpPacket receivedPacket) {
  loginfo_ln("[PAPA] got packet");

  if (receivedPacket.topic != reservedTopic::ack && 
      receivedPacket.topic != reservedTopic::rrep && 
      receivedPacket.topic != reservedTopic::rreq) {
    if(quackJson(receivedPacket) == -1) {
      if(packetQueue.size() > QUEUE_SIZE_MAX) {
        packetQueue.pop();
        packetQueue.push(receivedPacket.data);
      } else {
        packetQueue.push(receivedPacket.data);
      }
      Serial.print("[PAPA] New size of queue: ");
      loginfo_ln(packetQueue.size());
    }
  }
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
  // Initialize LED
  FastLED.addLeds<LED_TYPE, CDPCFG_PIN_LED1, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(BRIGHTNESS);
  leds[0] = CRGB::Cyan;
  FastLED.show();

  duck.setupWithDefaults();
  duck.joinWifiNetwork(WIFI_SSID, WIFI_PASSWORD);
  
  duck.onReceiveDuckData(handleDuckData);     // Callback handling incoming data from the network

  #ifdef CA_CERT
  loginfo_ln("[PAPA] Using root CA cert");
  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);
  #else
  loginfo_ln("[PAPA] Using insecure TLS");
  wifiClient.setInsecure();
  #endif
  
  loginfo_ln("[PAPA] Setup OK! ");
}

void loop() {
  
  if (!duck.isWifiConnected() && retry) {
    leds[0] = CRGB::Red;
    FastLED.show();
    duck.joinWifiNetwork(WIFI_SSID, WIFI_PASSWORD);
  }

  if (!client.loop()) {
    if(duck.isWifiConnected()) {
      leds[0] = CRGB::Gold;
      FastLED.show();
      mqttConnect();
    }
  }

  duck.run();
  timer.tick();

}

void mqttConnect() {
   if (!!!client.connected()) {
      loginfo_ln("[PAPA] Reconnecting MQTT client to %s",AWS_IOT_ENDPOINT);
      if(!!!client.connect(THINGNAME) && retry) {
        leds[0] = CRGB::Red;
        FastLED.show();
        loginfo_ln("[PAPA] Connection failed, retry in 5 seconds");
        retry = false;
        timer.in(5000, enableRetry);
      }
   } else {
    
      if(packetQueue.size() > 0) {
        leds[0] = CRGB::Gold;
        FastLED.show();
        publishQueue();
      }
   }
}

void subscribeTo(const char* topic) {
 loginfo_ln("[PAPA] subscribe to %s",topic);
 if (client.subscribe(topic)) {
   loginfo_ln(" OK");
 } else {
   loginfo_ln(" FAILED");
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
      loginfo_ln("[PAPA] Queue size: %i",packetQueue.size());
    } else {
      return;
    }
  }
}