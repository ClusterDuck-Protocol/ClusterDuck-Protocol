/**
 * @file Papa-DishDuck.ino
 * @brief Uses built-in PapaDuck from the SDK to create a WiFi enabled Papa Duck
 * 
 * This example will configure and run a Papa Duck that connect to the DMS-LITE over serial.
 * 
 * @date 2020-11-10
 * 
 */
#include <PapaDuck.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <IridiumSBD.h>
#include "timer.h"

#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14

//============== IRIDIUM Setup ================
#define IridiumSerial Serial2
#define DIAGNOSTICS false// Change this to see diagnostics
#define rxpin 25
#define txpin 13

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);
//============== IRIDIUM Setup ================

PapaDuck duck = PapaDuck();

#define SSID ""
#define PASSWORD ""

bool retry = true;

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
  duck.run();
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

  int err;

  std::string cdpTopic = toTopicString(packet.topic);

  std::string payload(packet.data.begin(), packet.data.end());
  std::string sduid(packet.sduid.begin(), packet.sduid.end());
  std::string muid(packet.muid.begin(), packet.muid.end());
  std::string path(packet.path.begin(), packet.path.end());

  //Uncomment the lines below of which data you want to send
  String data = sduid.c_str() + "/" + muid.c_str() + "/" + payload.c_str() +
                "/" + path.c_str() + "/" + cdpTopic.c_str();
  
  Serial.println(data);
  const   char *text = data.c_str();
  // Send the message
  Serial.print("Trying to send the message.  This might take several minutes.\r\n");
  err = modem.sendSBDText(text);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("sendSBDText failed: error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Try again with a better view of the sky.");
  }
  else
  {
    Serial.println("Message Sent to Iridium!");
  }
}

void setupRockBlock(){
  int signalQuality = -1;
  int err;
  
  // Start the serial port connected to the satellite modem
  Serial2.begin(19200, SERIAL_8N1, rxpin, txpin, false); // false means normal data polarity so steady state of line = 0, true means staedy state = high.
  // Begin satellite modem operation
  Serial.println("Starting modem...");
  err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print("Modem begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
    return;
  }

  //============== Print Firmware Version ================
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
    Serial.print("FirmwareVersion failed: error ");
    Serial.println(err);
    return;
  }

  {
  Serial.print("Firmware Version is ");
  Serial.print(version);
  Serial.println(".");
  }
//============== Print Firmware Version ================ 
}
