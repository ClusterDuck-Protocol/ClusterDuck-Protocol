#include <ClusterDuck.h>
//#include "timer.h"

//auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;

bool runSensor(void *) {

  return true;
}

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("ABC");
  duck.setupMamaDuck();

  //timer.every(300000, runSensor);
}

void loop() {
  //timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
}

