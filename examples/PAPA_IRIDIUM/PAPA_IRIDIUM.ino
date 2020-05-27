#include <ClusterDuck.h>
#include <PubSubClient.h>
#include <IridiumSBD.h>
#include <ArduinoJson.h>
#include "timer.h"
ClusterDuck duck;
auto timer = timer_create_default(); // create a timer with default settings
byte ping = 0xF4;
#define IridiumSerial Serial2
#define DIAGNOSTICS false// Change this to see diagnostics
#define rxpin 25
#define txpin 13

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);

void setup() {
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Papa");
  duck.setupDisplay("Papa");
  setupRockBlock();
  duck.setupLoRa();
//  Serial.println("PAPA Online");
}

void loop() {
  if(duck.getFlag()) {  //If LoRa packet received
    duck.flipFlag();
    duck.flipInterrupt();
    int pSize = duck.handlePacket();
    if(pSize > 3) {
     duck.getPacketData(pSize);
      quackJson();
    }
    duck.flipInterrupt();
    duck.startReceive();
  }
  timer.tick();
}
void quackJson() {
  int err;
  Packet lastPacket = duck.getLastPacket();
  
  //Uncomment the lines below of which data you want to send
   String data = lastPacket.senderId +  "/" + lastPacket.messageId + "/"+ lastPacket.payload + "/"+  lastPacket.path + "," + duck.getDeviceId() ;
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
    Serial.println("Hey, it worked!");
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
  // Example: Print the firmware revision
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
  
}
