#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

//Dust setup
#include "WaveshareSharpDustSensor.h"
const int iled = 22; //drive the led of sensor
const int vout = 0;  //analog input
int   adcvalue;
WaveshareSharpDustSensor ds;

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Z", 18);
  duck.setupMamaDuck();

  //Dust sensor
  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW);  //iled default closed

  timer.every(150000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
}

bool runSensor(void *) {

  //Dust sensor
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);  
  adcvalue = ds.Filter(adcvalue);
  float density = ds.Conversion(adcvalue);
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");

  String sensorVal = "Current dust concentration: " + String(density/2500);

  duck.sendPayloadMessage(sensorVal);

  return true;
}
