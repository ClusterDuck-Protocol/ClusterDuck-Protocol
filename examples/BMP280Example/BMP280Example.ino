#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

//Setup BMP280
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp;

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Z", 18);
  duck.setupMamaDuck();

  //Temperature and pressure
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP208 detected ... Check your wiring or I2C ADDR!");
    while(1); 
  }

  timer.every(150000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
}

bool runSensor(void *) {
  float T,P;
  
  T = bmp.readTemperature();
  Serial.println(T);
  P = bmp.readPressure();
  Serial.println(P);
  
  payload.sensorVal = "Temp: " + String(T) + " Pres: " + String(P); //Store Data

  return true;
}
