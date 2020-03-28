#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

//Setup BMP180
#include <Adafruit_BMP085_U.h>
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

ClusterDuck duck;

bool runSensor(void *) {
  float T,P;
  
  bmp.getTemperature(&T);
  Serial.println(T);
  bmp.getPressure(&P);
  Serial.println(P);
  
  String sensorVal = "Temp: " + String(T) + " Pres: " + String(P);

  duck.sendPayloadMessage(sensorVal);

  
  return true;
}

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Z");
  duck.setupMamaDuck();

  //Temperature and pressure
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  } else {
    Serial.println("BMP on");
  }

  timer.every(150000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
}


