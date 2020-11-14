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

// Use pre-built papa duck: the duck ID is provided by DMS
PapaDuck duck = PapaDuck("PAPA");
 



bool retry = true;

void setup() {
  duck.setupSerial(115200);
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_DIO1_PIN, LORA_TXPOWER);
  setupRockBlock();
  
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

 int err;
  Packet lastPacket = packet;
  
  //Uncomment the lines below of which data you want to send
   String data = lastPacket.senderId +  "/" + lastPacket.messageId + "/"+ lastPacket.payload + "/"+  lastPacket.path + "/" + lastPacket.topic;
  // String data = lastPacket.messageId ;
  
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
