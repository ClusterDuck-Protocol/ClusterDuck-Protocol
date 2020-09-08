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
  
  //Get flag if LoRa packet received
  if(duck.getFlag()) {

    //Flip lora flag for next message
    duck.flipFlag(); 

    //Flip interrupt to false
    duck.flipInterrupt(); 

    //Read and store lora packet
    int pSize = duck.handlePacket(); 
    Serial.print("runMamaDuck pSize ");
    Serial.println(pSize);

    //Make sure the packet has data
    if(pSize > 0) { 

      //Decode packet data and store in _lastPacket
      String msg = duck.getPacketData(pSize); 

      //Get _lastPacket data
      Packet lastPacket = duck.getLastPacket(); 

      //Check if device ID is in path
      if(msg != "ping" && !duck.idInPath(lastPacket.path)) { 
        Serial.println("runMamaDuck relayPacket");

        //Transmit received packet
        duck.sendPayloadStandard(lastPacket.payload, lastPacket.topic, lastPacket.senderId, lastPacket.messageId, lastPacket.path); 

      }
    }

    //Flip interrupt to true
    duck.flipInterrupt(); 
    Serial.println("runMamaDuck startReceive");

    //Turn on Receiver
    duck.startReceive(); 
  }
  //Handle submits on captive portal
  duck.processPortalRequest();
  
  
}

//Use this to run code on timer
bool runSensor(void *) {

  return true;
}
