/**
 * @file Serial-PaPiDuckExample.ino
 * @author 
 * @brief Uses built-in PapaDuck from the SDK to create a Serial Print papaDuck for DMS Lite
 * 
 * This example will configure and run a Papa Duck that will print JSON in the serial monitor for the DMS-Lite to pick up
 * 
 * @date 2021-02-08 
 * 
 */

#include <ArduinoJson.h>
#include <arduino-timer.h>
#include <string>

/* CDP Headers */
#include <PapaDuck.h>
#include <CdpPacket.h>

#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14

#define CMD_STATE_WIFI "/wifi/" 
#define CMD_STATE_HEALTH "/health/"
#define CMD_STATE_CHANNEL "/channel/"
#define CMD_STATE_MBM "/messageBoard/"

// Use pre-built papa duck
PapaDuck duck;

DuckDisplay* display = NULL;

// Set this to false if testing quickstart on IBM IoT Platform
bool use_auth_method = true;

auto timer = timer_create_default();

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

void quackJson(std::vector<byte> packetBuffer) {

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

  doc["DeviceID"] = sduid;
  doc["MessageID"] = muid;
  doc["Payload"].set(payload);
  doc["path"].set(path);
  doc["hops"].set(packet.hopCount);
  doc["duckType"].set(packet.duckType);
  doc["topic"].set(toTopicString(packet.topic));

  String jsonstat;
  serializeJson(doc, Serial);
  Serial.println("");
//  serializeJsonPretty(doc, Serial);
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(std::vector<byte> packetBuffer) {

  quackJson(packetBuffer);
}

//executes Duck Commands (identical to IBM cloud version)
void gotMsg(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("gotMsg: invoked for topic: "); Serial.println(topic);
 
  if (String(topic).indexOf(CMD_STATE_WIFI) > 0) {
    Serial.println("Start WiFi Command");
    byte sCmd = 1;
    std::vector<byte> sValue = {payload[0]};

    if(payloadLength > 3) {
      std::string destination = "";
      for (int i=1; i<payloadLength; i++) {
        destination += (char)payload[i];
      }
      std::vector<byte> dDevId;
      dDevId.insert(dDevId.end(),destination.begin(),destination.end());
      
      duck.sendCommand(sCmd, sValue, dDevId);
    } else {
      duck.sendCommand(sCmd, sValue);
    }
  } else if (String(topic).indexOf(CMD_STATE_HEALTH) > 0) {
    byte sCmd = 0;
    std::vector<byte> sValue = {payload[0]};
    if(payloadLength >= 8) {
      std::string destination = "";
      for (int i=1; i<payloadLength; i++) {
        destination += (char)payload[i];
      }
      
      std::vector<byte> dDevId;
      dDevId.insert(dDevId.end(),destination.begin(),destination.end());
      
      duck.sendCommand(sCmd, sValue, dDevId);
      
    } else {
      Serial.println("Payload size too small");
    }
  } else if (String(topic).indexOf(CMD_STATE_MBM) > 0){
      std::vector<byte> message;
      std::string output;
      for (int i = 0; i<payloadLength; i++) {
        output = output + (char)payload[i];
      }
   
      message.insert(message.end(),output.begin(),output.end());
      duck.sendMessageBoardMessage(message);
  } else {
    Serial.print("gotMsg: unexpected topic: "); Serial.println(topic); 
  } 
}

//Reads Commands from Serial and passes them on to gotMsg()
void readCommands() {

   if (Serial.available())
  {
    String command = Serial.readString();
    DynamicJsonDocument doc(65536);
    deserializeJson(doc, command);

    char str_payload[strlen(doc["payload"])];
    strcpy(str_payload,doc["payload"]);
    String payload = String(str_payload);
     
    char topic[strlen(doc["topic"])];
    strcpy(topic,doc["topic"]);
      
    Serial.print("Handling Command: " + command);
    gotMsg(topic,(byte*)payload.c_str(),payload.length());
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

  //Set the Device ID
  duck.setDeviceId(devId);

  // the default setup is equivalent to the above setup sequence
// duck.setupSerial(115200);
  Serial.begin(115200);
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN,
                  LORA_DIO1_PIN, LORA_TXPOWER);
  duck.setDeviceId(devId);

  duck.enableAcks(true);
 
  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);

}

void loop() {
  duck.run();
  readCommands();
  timer.tick();
}
