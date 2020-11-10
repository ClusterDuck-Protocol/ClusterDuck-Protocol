/**
 * @file DustSensorExample.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * A MamaDuck that sends sensor data from a Dust sensor
 * 
 * @date 2020-11-10
 * 
 * @copyright Copyright (c) 2020
 * ClusterDuck Protocol 
 */

#include "timer.h"
#include <MamaDuck.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

//Include for DustSensor
#include <GP2YDustSensor.h>

const uint8_t SHARP_LED_PIN = 22;   // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = 0;    // Sharp Dust/particle analog out pin used for reading 

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);


// Set device ID between ""
MamaDuck duck = MamaDuck("DuckOne");

auto timer = timer_create_default();
const int INTERVAL_MS = 60000;
char message[32]; 
int counter = 1;

void setup() {
  // Use the default setup provided by the SDK
  duck.setupWithDefaults();
  Serial.println("MAMA-DUCK...READY!");

    //Dust sensor
  dustSensor.begin();

  // initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);
}

void loop() {
  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();
}

bool runSensor(void *) {

  //Dust sensor
  String sensorVal = "Current dust concentration: " + dustSensor.getDustDensity();
  sensorVal += " ug/m3";

  duck.sendPayloadStandard(sensorVal, "dust");
  
  return true;
}