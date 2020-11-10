/**
 * @file DuckLink.ino
 * @brief Base Node in the CDP network
 *
 * This example creates a duck link that sends a counter message periodically
 * It's using a pre-built ducklink available from the ClsuterDuk SDK 
 *
 * @date 2020-11-10
 *
 * @copyright Copyright (c) 2020
 * ClusterDuck Protocol 
 */

#include "timer.h"
#include <DuckLink.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a duck
DuckLink duck = DuckLink("DUCKLINK-SAMD");

auto timer = timer_create_default(); // create a timer with default settings
const int INTERVAL_MS = 60000;
char message[32]; 
int counter = 1;

void setup() {
  // Setup a DuckLink with default settings:
  // It only has serial and radio components
  duck.setupWithDefaults();        
  
  timer.every(INTERVAL_MS, runSensor);

  Serial.println(F("[setup] DUCKLINK is ready!"));

}

void loop() {
  timer.tick();
  duck.run();
}

bool runSensor(void *) {
  sprintf(message, "ducklink counter %d", counter++); 
  duck.sendPayloadStandard(message, "counter-msg"); // sender id will be populated automatically
  
  return true;
}
