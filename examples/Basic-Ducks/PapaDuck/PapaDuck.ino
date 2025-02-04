/**
 * @file PapaDuck-Basic.ino
 * @author Amir Nathoo
 * @brief 
 *  This basic PapaDuck example can communicate with other ducks but does connect to the cloud.
 *  It will instead output messages to the serial monitor in JSON format.
 * 
 * 
 * @date 02-28-2024
 *
 */

#include <CDP.h>

#include <ArduinoJson.h>
#include <arduino-timer.h>
#include <string>

#include <queue>
#include <iomanip>
#include <sstream>


std::string toTopicString(byte topic);
std::string convertToHex(byte* data, int size);
int toJSON(CdpPacket packet);


const int bufferSize = 4 * JSON_OBJECT_SIZE(4);
StaticJsonDocument<bufferSize> doc;

bool setupOK = false;

// Use pre-built papa duck
PapaDuck duck;

auto timer = timer_create_default();


std::string toTopicString(byte topic) 
{

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

std::string convertToHex(byte* data, int size) 
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

int toJSON(CdpPacket packet) 
{
  
  std::stringstream ss;

  // convert the dduid to a string. We should probably do this for all fields
  // using stringstream because print will interpret 00 as a null terminator
  for (auto &c : packet.dduid) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
  }
  
  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string dduid = ss.str();
  std::string muid(packet.muid.begin(), packet.muid.end());

  loginfo_ln("[PAPA] topic:   %s", toTopicString(packet.topic).c_str());
  
  loginfo_ln("[PAPA] sduid:   %s", sduid.c_str());
  loginfo_ln("[PAPA] dduid:   %s", dduid.c_str());

  loginfo_ln("[PAPA] muid:    %s", muid.c_str());
  loginfo_ln("[PAPA] data:    %s", payload.c_str());
  loginfo_ln("[PAPA] hops:    %s",  packet.hopCount);
  loginfo_ln("[PAPA] duck:    %s", packet.duckType);

  doc["DeviceId"] = sduid;
  doc["topic"].set(toTopicString(packet.topic));
  doc["MessageId"] = muid;
  doc["Payload"].set(payload);
  doc["hops"].set(packet.hopCount);
  doc["duckType"].set(packet.duckType);

  std::string jsonstat;
  serializeJson(doc, jsonstat);
  serializeJsonPretty(doc, Serial);
  Serial.print("\n[PAPA] --------------------------------------------------------------------------------------\n\n");

  
  return 1;
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(std::vector<byte> packetBuffer)
{
  Serial.print("\n[PAPA] --------------------------------------------------------------------------------------\n");
  loginfo_ln("[PAPA] got packet: %s", convertToHex(packetBuffer.data(), packetBuffer.size()).c_str());

  CdpPacket packet = CdpPacket(packetBuffer);
  toJSON(packet);
}

void setup() 
{
  std::string deviceId("PAPADUCK");
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[PAPA] Failed to setup MamaDuck");
    setupOK = false;
    return;
  }
  setupOK = true;
  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);

  Serial.println("[PAPA] Setup OK! ");

  duck.enableAcks(false);
}

void loop() 
{
  if (!setupOK) {
    return;
  }
  duck.run();
  timer.tick();
}
