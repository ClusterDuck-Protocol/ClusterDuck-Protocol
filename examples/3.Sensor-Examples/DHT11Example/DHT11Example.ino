/**
 * @file DHT11Example.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * A MamaDuck that sends sensor data from a DHT11 sensor
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


//Setup DHT11
#include <DHT.h>
#define DHTTYPE DHT11
#define DHTPIN  23
#define TIMEOUT 5000
DHT dht(DHTPIN, DHTTYPE);
 
// Set DuckId between ""
MamaDuck duck = MamaDuck("DHT-DUCK");

auto timer = timer_create_default();
const int INTERVAL_MS = 10000;
char message[32]; 
int counter = 1;

void setup() {
  // Use the default setup provided by the SDK
  duck.setupWithDefaults();
  Serial.println("MAMA-DUCK...READY!");

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

  String sensorVal = "Temp: ";
  sensorVal += dht.readTemperature();
  sensorVal += "Humidity: ";
  sensorVal += dht.readHumidity();

  duck.sendPayloadStandard(sensorVal, "DHT");
  
  return true;
}