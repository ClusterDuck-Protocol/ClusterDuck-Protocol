/**
 * @file Serial-PaPiDuckExample.ino
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

#include "timer.h"

#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14

// Use pre-built papa duck: the duck ID is provided by DMS
PapaDuck duck = PapaDuck("PAPA");
 



bool retry = true;

void setup() {
  duck.setupSerial(115200);
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_DIO1_PIN, LORA_TXPOWER);
  
  // register a callback to handle incoming data from duck in the network
  duck.onReceiveDuckData(handleDuckData);
  
  Serial.println("[PAPA] Setup OK!");
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over WiFi
void handleDuckData(Packet packet) {
  quackJson(packet);
}

void loop() {


  duck.run();

}


void quackJson(Packet packet) {

  if (packet.topic == "ping") {
    return;
  }
  
  const int bufferSize = 4 *  JSON_OBJECT_SIZE(4);
  StaticJsonDocument<bufferSize> doc;

  doc["DeviceID"]  = packet.senderId;
  doc["MessageID"] = packet.messageId;
  doc["Payload"].set(packet.payload);

  // FIXME: Path shouldn't be altered by the application
  // It should done in the library PapaDuck component
  doc["path"].set(packet.path + "," + "PAPA");



  String jsonstat;
//  serializeJson(doc, jsonstat);
  serializeJsonPretty(doc, Serial);

}
