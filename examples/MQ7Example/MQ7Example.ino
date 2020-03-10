#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

//Setup MQ7
#include "MQ7.h" //https://github.com/swatish17/MQ7-Library
#define analogMQ7  32            //GPIO PIN fot MQ
MQ7 mq7(analogMQ7,5.0); //Setting up to 5V the MQ Sensor

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Z", 18);
  duck.setupMamaDuck();

 
  

  timer.every(15000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
}

bool runSensor(void *) {

 String sensorVal = "CO: ";
 sensorVal += mq7.getPPM();



  duck.sendPayloadMessage(sensorVal);

  
  return true;
}
