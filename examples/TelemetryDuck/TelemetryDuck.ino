/* 
   this is a mamaduck that sends some basic data quacks every now and then
   it is useful as a quickstart example since it doesnt need any configuration
   it reports some basic/crude stats like 
   - cpu situation (millis+loopcount)
   - memory situation (free and fragmentation)
   - temperature (from sx127x sensor)
   - battery level (from battery pin)

*/

// pin definitions for "heltec lora v2"
#define PIN_BAT  37
#define PIN_VEXT 21

// this is currently required to access the sx127x temperature sensor
#include <RadioLib.h>
extern SX1276 lora;

#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

// the ID will be autoreplaced by the last four digits of the MAC
String myid = "blah";

ClusterDuck duck;

void setup() {
  // use last 16 bit of MAC as duck-id
  uint64_t chipid = ESP.getEfuseMac();
  myid = String((uint16_t)(chipid>>32),HEX);

  // switch on vext (so we can read the battery voltage)
  pinMode(PIN_VEXT,OUTPUT);
  digitalWrite(PIN_VEXT,LOW);

  // standard mama setup
  duck.begin();
  duck.setDeviceId(myid);
  duck.setupMamaDuck();

  // dont send a quack right at startup 
  // this avoids spectrum spam in brownout rapid-reboot-loop situations
  schedSensor();
  Serial.println("SETUP DONE");
}

void schedSensor() {
  // send a sensor quack every 45-75 seconds
  timer.in(45000+random(30000), runSensor);
}

uint32_t loopCount = 0;
void loop() {
  loopCount++;
  timer.tick();
  duck.runMamaDuck();
}

bool runSensor(void *) {

  uint16_t raw = analogRead(PIN_BAT);

  int8_t temp = lora.getTempRaw();

  String sensorVal = "myid:" + myid + " millis:" + String(millis(),DEC) + " lc:" + String(loopCount,DEC) +
	" bat:" + String(raw,DEC) + 
	" gFH:" + String(ESP.getFreeHeap(),DEC) +
	" gMFH:" + String(ESP.getMinFreeHeap(),DEC) +
	" gMAH:" + String(ESP.getMaxAllocHeap(),DEC) +
	" temp:" + String(temp,DEC);

  schedSensor();

  Serial.println("SENSOR: " + sensorVal);
  duck.sendPayloadMessage(sensorVal);
  
  return true;
}
