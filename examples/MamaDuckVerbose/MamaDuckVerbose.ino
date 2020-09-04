#include <ClusterDuck.h>
//#include "timer.h"

//auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  //Begin serial for debug
  duck.begin();
  //Set device id, each device must have a unique id in a network
  duck.setDeviceId("R");
  //Setup display if there is a led screen, takes a string to display device type
  duck.setupDisplay("Mama");
  //Setup lora
  duck.setupLoRa();
  //Sets up wifi access point
  duck.setupWifiAp();
  //Setup DNS and IP
  duck.setupDns();
  //Setup web server and captive portal
  duck.setupWebServer(true);
  //Initialize ArduinoOTA for over the air updates
  duck.setupOTA();

  Serial.println("MamaDuck Online");

  //Set timer to run code on specified interval
  //timer.every(300000, runSensor);
}

void loop() {
  //Advance timer
  //timer.tick();
  
  //Handle OTA requests
  ArduinoOTA.handle();

  if(duck.getFlag()) {  //Get flag if LoRa packet received
    duck.flipFlag(); //Flip lora flag for next message
    duck.flipInterrupt(); //Flip interrupt to false
    int pSize = duck.handlePacket(); //Read and store lora packet
    Serial.print("runMamaDuck pSize ");
    Serial.println(pSize);
    if(pSize > 0) { //Make sure the packet has data
      String msg = duck.getPacketData(pSize); //Decode packet data and store in _lastPacket
      Packet lastPacket = duck.getPacketData(); //Get _lastPacket data
      if(msg != "ping" && !duck.idInPath(lastPacket.path)) { //Check if device ID is in path
        Serial.println("runMamaDuck relayPacket");
        duck.sendPayloadStandard(lastPacket.payload, lastPacket.topic, lastPacket.senderId, lastPacket.messageId, lastPacket.path); //Transmit received packet

      }
    }

    duck.flipInterrupt(); //Flip interrupt to true
    Serial.println("runMamaDuck startReceive");
    duck.startReceive(); //Turn on Receiver
  }

  duck.processPortalRequest();
  
  
}

bool runSensor(void *) {

  return true;
}
