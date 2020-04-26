#include <ClusterDuck.h>
#include "timer.h"

ClusterDuck duck;

auto timer = timer_create_default(); // create a timer with default settings
bool ponger = false;

void setup() {
   duck.begin();
   duck.setDeviceId("Det");
   duck.setupLED();
   duck.setupDetect();

   duck.setupLED();

   timer.every(3000, ping);
  
}

void loop() {
   timer.tick();

   if(duck.getFlag()) {  //If LoRa packet received
      int val = duck.runDetect();
      if(val != 0) {
        ledOn(val);
        ponger = true;
      }
   }

}

bool ping(void *) {
   Serial.print("Ping!");
   if(ponger) {
      ponger = false;
      Serial.print("Pong!");
   }
   else {
      duck.setColor(0,0,25);
   }

   duck.ping();
   duck.startReceive();

   return true;
}

void ledOn(int incoming) {
   Serial.println("Run LED");
   Serial.println(incoming);
   int rssi = incoming;
   Serial.print(rssi);
   
   if(rssi > -95) {
      duck.setColor(0,50,0);
      Serial.println("Rssi good");
   }
   else if(rssi <= -95 && rssi > -108) {
      duck.setColor(25,0,50);
      Serial.println("Rssi okay");
   }
   else if(rssi <= -108) {
      duck.setColor(50,0,0);
      Serial.println("Rssi bad");
   }
}


