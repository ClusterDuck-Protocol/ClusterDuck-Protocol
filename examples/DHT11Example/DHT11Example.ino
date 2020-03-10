#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

//Setup DHT11
#include <DHT.h>
#define DHTTYPE DHT11
#define DHTPIN  23
#define TIMEOUT 5000
DHT dht(DHTPIN, DHTTYPE);

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  dht.begin();
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

 String sensorVal = "Temp: ";
  sensorVal += dht.readTemperature();
  sensorVal += "Humidity: "
  sensorVal += dht.readHumidity();


  duck.sendPayloadMessage(sensorVal);

  
  return true;
}
