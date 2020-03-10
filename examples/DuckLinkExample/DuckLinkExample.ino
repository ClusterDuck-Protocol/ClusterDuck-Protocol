#include <ClusterDuck.h>
//#include "timer.h"

//auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("D", 18);
  duck.setupDuckLink();

  //timer.every(300000, runCode);
}

void loop() {
  //timer.tick();
  
  // Run captive portal and LoRa in the DuckLink configuration
  duck.runDuckLink();
  
}

// Use this with the timer to run code at a timed interval
// Works well with taking sensor readings
bool runCode(void *) {

  return true;
}