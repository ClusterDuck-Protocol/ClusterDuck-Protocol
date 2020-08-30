#include "timer.h"
#include <ClusterDuck.h>

/**
 * This example creates a mama duck that sends a counter message
 * every 30 seconds towards a papa duck.
 * For sensor based examples see: ClusterDuck-Protocol/examples/SensorExamples
 */

ClusterDuck duck;

auto timer = timer_create_default(); // create a timer with default settings
const int INTERVAL_MS = 30000;
char message[32]; 
int counter = 1;

void setup() {
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("MAMA-DUCK");
  duck.setupMamaDuck();
  timer.every(INTERVAL_MS, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
}

bool runSensor(void *) {
  sprintf(message, "Mama duck counter %d", counter++); 
  duck.sendPayloadStandard(message, "message-test", "MAMA-DUCK");
  
  return true;
}
