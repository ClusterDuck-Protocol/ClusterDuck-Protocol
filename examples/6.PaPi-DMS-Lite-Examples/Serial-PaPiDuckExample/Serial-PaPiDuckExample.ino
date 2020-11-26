/**
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 *
 * This example will configure and run a Papa Duck that connects to the cloud
 * and forwards all messages (except  pings) to the cloud.
 *
 * @date 2020-09-21
 *
 */

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <string>

/* CDP Headers */
#include <CdpPacket.h>
#include <PapaDuck.h>

#define MQTT_RETRY_DELAY_MS 500
#define WIFI_RETRY_DELAY_MS 5000

#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14

#define MQTT_RETRY_DELAY_MS 500
#define WIFI_RETRY_DELAY_MS 5000

// Use pre-built papa duck
PapaDuck duck = PapaDuck();

bool retry = true;

void setup() {

  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning
  // then converted to a byte vector to setup the duck
  // NOTE: The Device ID must be exactly 8 bytes otherwise it will get rejected
  std::string deviceId(DEVICE_ID);
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  duck.setupSerial(115200);
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN,
                  LORA_DIO1_PIN, LORA_TXPOWER);
  duck.setDeviceId(devId);

  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);

  Serial.println("[PAPA] Setup OK!");
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(Packet packet) {
  if (packet.topic == reservedTopic::ping) {
    return;
  }
  quackJson(packet);
}

void loop() { duck.run(); }

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

void quackJson(Packet packet) {

  const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  std::string payload(packet.data.begin(), packet.data.end());
  std::string duid(packet.sduid.begin(), packet.sduid.end());
  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  doc["DeviceID"] = duid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["path"].set(path);

  std::string cdpTopic = toTopicString(packet.topic);

  std::string topic = "iot-2/evt/" + cdpTopic + "/fmt/json";

  serializeJsonPretty(doc, Serial);
}
