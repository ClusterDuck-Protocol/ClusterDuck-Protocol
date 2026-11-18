/**
 * @file AWS-PapaDuck.ino
 * @author Timo Wielink
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 *
 * This example will configure and run a Papa Duck that connects to AWS cloud
 * and forwards all messages (except pings, pongs and other reserved topics) to
 * the cloud. When disconnected it will add received packets to a queue. When it
 * reconnects to MQTT it will try to publish all messages in the queue. You can
 * change the size of the queue by changing `QUEUE_SIZE_MAX`.
 *
 * While it connects to AWS, the PapaDuck doesn't receive, and LoRa packets that
 * arrive then can be lost. MQTT_TIMEOUT_S keeps each attempt short, and after
 * each failed attempt it waits twice as long before the next one, up to
 * MQTT_RETRY_MAX_MS. WiFi reconnects by itself.
 *
 * CDP 5.5: the receive callback gets every copy of a message (one per relay
 * path), and every copy is forwarded; remove duplicates in the cloud by
 * MessageID. Every 15 minutes the PapaDuck's own health (topics::health) also
 * arrives in the callback, so it is forwarded like the health of other ducks.
 *
 * @date 05-07-2025
 */
#include <algorithm>
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

// Setup for W2812 (LED), on boards that define CDPCFG_PIN_LED1
#ifdef CDPCFG_PIN_LED1
#include <FastLED.h>
#include <pixeltypes.h>
#define LED_TYPE WS2812
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
CRGB leds[NUM_LEDS];
#endif

// Show a colour on the LED. Does nothing on boards without one.
void setLed(uint32_t color) {
#ifdef CDPCFG_PIN_LED1
  leds[0] = CRGB(color);
  FastLED.show();
#else
  (void)color;
#endif
}

// --- Global Objects ---
PapaDuck duck(THINGNAME);
size_t QUEUE_SIZE_MAX = 5;
auto timer = timer_create_default();
bool setupOK = false;
const unsigned long MQTT_CHECK_MS = 5000;       // time between two connection checks
const unsigned long MQTT_RETRY_MAX_MS = 60000;  // longest wait after failed attempts
const uint16_t MQTT_TIMEOUT_S = 5;              // longest wait for each connection step
unsigned long mqttRetryMs = MQTT_CHECK_MS;      // wait before the next connection check

// --- Function Declarations ---
std::queue<CdpPacket> packetQueue;
int quackJson(CdpPacket packet);
void handleDuckData(CdpPacket receivedPacket);
bool mqttConnect(void*);
void subscribeTo(const char* topic);
void publishQueue();

// --- WIFI Setup Function ---
WiFiClientSecure wifiClient;
PubSubClient client(AWS_IOT_ENDPOINT, 8883, wifiClient);
// PubSubClient's default 256-byte buffer is too small for a 228-byte CDP
// payload wrapped in JSON, and publish() fails when it doesn't fit. JSON
// writes a control byte, such as 0x00, as 6 characters, so a binary payload
// can need about 1.6 KB.
const uint16_t MQTT_BUFFER_SIZE = 2048;

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
 * This function is invoked when a packet is received, and for the PapaDuck's
 * own health message. It publishes the packet unless it is on a reserved topic
 * (ping, pong), and queues it for later when publishing fails.
 *
 * @param receivedPacket The received packet
 */
void handleDuckData(CdpPacket receivedPacket) {
  loginfo_ln("[PAPA] got packet");

  if (receivedPacket.topic >= reservedTopic::max_reserved) {
    if(quackJson(receivedPacket) == -1) {
      if(packetQueue.size() >= QUEUE_SIZE_MAX) {
        packetQueue.pop();
      }
      packetQueue.push(receivedPacket);
      loginfo_ln("[PAPA] New size of queue: %u", packetQueue.size());
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
#ifdef CDPCFG_PIN_LED1
  FastLED.addLeds<LED_TYPE, CDPCFG_PIN_LED1, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(BRIGHTNESS);
#endif
  setLed(0x00FFFF); // cyan

  int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("[PAPA] Setup failed, rc = %d (%s)", err, getDuckErrorString(err));
    setLed(0xFF0000); // red
    return;
  }
  duck.onReceiveDuckData(handleDuckData);     // Callback handling incoming data from the network
  setupOK = true;

  // Retries a few times when the network isn't found, so this can take ~30 s.
  // After that, the ESP32 reconnects to WiFi by itself.
  duck.joinWifiNetwork(WIFI_SSID, WIFI_PASSWORD);

  #ifdef CA_CERT
  loginfo_ln("[PAPA] Using root CA cert");
  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);
  #else
  loginfo_ln("[PAPA] Using insecure TLS");
  wifiClient.setInsecure();
  #endif
#if ESP_ARDUINO_VERSION_MAJOR < 3
  wifiClient.setTimeout(MQTT_TIMEOUT_S);           // TCP connect, in seconds (ESP32 core 2.x)
#endif
  wifiClient.setHandshakeTimeout(MQTT_TIMEOUT_S);  // TLS handshake, in seconds
  client.setBufferSize(MQTT_BUFFER_SIZE);
  client.setSocketTimeout(MQTT_TIMEOUT_S);         // wait for the broker's answer
  mqttConnect(nullptr);   // connects, and schedules the next check

  loginfo_ln("[PAPA] Setup OK! ");
}

void loop() {
  if (!setupOK) {
    return;
  }
  duck.run();
  timer.tick();
  client.loop();
}

/**
 * @brief Connect to AWS IoT if WiFi is up and MQTT isn't, publish the queue
 * once connected, then schedule the next check. Called once from setup() and
 * then by the timer. Each failed attempt doubles the wait before the next one,
 * up to MQTT_RETRY_MAX_MS, because duck.run() doesn't run during an attempt.
 * @returns false: this is a one-shot timer, and the next one is set here
 */
bool mqttConnect(void*) {
  if (!duck.isWifiConnected()) {
    setLed(0xFF0000); // red
  } else if (!client.connected()) {
    setLed(0xFFD700); // gold
    loginfo_ln("[PAPA] Connecting MQTT client to %s", AWS_IOT_ENDPOINT);
    if (client.connect(THINGNAME)) {
      loginfo_ln("[PAPA] MQTT connected");
      mqttRetryMs = MQTT_CHECK_MS;
    } else {
      setLed(0xFF0000); // red
      mqttRetryMs = std::min(2 * mqttRetryMs, MQTT_RETRY_MAX_MS);
      loginfo_ln("[PAPA] Connection failed, state = %d, next attempt in %lu s",
        client.state(), mqttRetryMs / 1000);
    }
  }

  if (client.connected() && !packetQueue.empty()) {
    publishQueue();
  }
  timer.in(mqttRetryMs, mqttConnect);
  return false;
}

void subscribeTo(const char* topic) {
 loginfo_ln("[PAPA] subscribe to %s",topic);
 if (client.subscribe(topic)) {
   loginfo_ln(" OK");
 } else {
   loginfo_ln(" FAILED");
 }
}

void publishQueue() {
  while(!packetQueue.empty()) {
    if(quackJson(packetQueue.front()) == 0) {
      packetQueue.pop();
      loginfo_ln("[PAPA] Queue size: %u", packetQueue.size());
    } else {
      return;
    }
  }
}