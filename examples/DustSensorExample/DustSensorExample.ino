#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

#include <GP2YDustSensor.h>

const uint8_t SHARP_LED_PIN = 22;   // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = 0;    // Sharp Dust/particle analog out pin used for reading 

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);

MamaDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Z");
  duck.setup();

  //Dust sensor
  dustSensor.begin();

  timer.every(150000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.run();
  
}

bool runSensor(void *) {

  //Dust sensor
  String sensorVal = "Current dust concentration: " + dustSensor.getDustDensity();
  sensorVal += " ug/m3";

  duck.sendPayloadMessage(sensorVal);

  return true;
}