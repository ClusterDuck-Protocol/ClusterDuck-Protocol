#include <ClusterDuck.h>
//#include "timer.h"

//auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("R");
  duck.setupDisplay("Mama");
  duck.setupLoRa();
  duck.setupWifiAp();
  duck.setupDns();
  duck.setupWebServer(true);
  duck.setupOTA();

  Serial.println("MamaDuck Online");

  //timer.every(300000, runSensor);
}

void loop() {
  //timer.tick();
  
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();

  if(duck.getFlag()) {  //If LoRa packet received
    duck.flipFlag();
    duck.flipInterrupt();
    int pSize = duck.handlePacket();
    Serial.print("runMamaDuck pSize ");
    Serial.println(pSize);
    if(pSize > 0) {
      String msg = duck.getPacketData(pSize);
      Packet lastPacket = duck.getPacketData();
      if(msg != "ping" && !duck.idInPath(lastPacket.path)) {
        Serial.println("runMamaDuck relayPacket");
        duck.sendPayloadStandard(lastPacket.payload, lastPacket.topic, lastPacket.senderId, lastPacket.messageId, lastPacket.path);

      }
    }

    duck.flipInterrupt();
    Serial.println("runMamaDuck startReceive");
    duck.startReceive();
  }

  duck.processPortalRequest();
  
  
}

bool runSensor(void *) {

  return true;
}
