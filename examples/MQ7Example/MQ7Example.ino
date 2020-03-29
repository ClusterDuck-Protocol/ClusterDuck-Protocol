#include <ClusterDuck.h>
#include "timer.h"

auto timer = timer_create_default(); // create a timer with default settings

//Include the library
#include <MQUnifiedsensor.h>

//Definitions
#define pin A0 //Analog input 0 of your arduino
#define type 7 //MQ7
//#define calibration_button 13 //Pin to calibrate your sensor

//Declare Sensor
MQUnifiedsensor MQ7(pin, type);

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("Z");
  duck.setupMamaDuck();

  //init the sensor
  /*****************************  MQInicializar****************************************
  Input:  pin, type 
  Output:  
  Remarks: This function create the sensor object.
  ************************************************************************************/ 
  MQ7.inicializar(); 
  //pinMode(calibration_button, INPUT);

  timer.every(15000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
}

bool runSensor(void *) {

  MQ7.update();

  String sensorVal = "H2: ";
  sensorVal += MQ7.readSensor("H2");
  sensorVal += " LPG: ";
  sensorVal += MQ7.readSensor("LPG");
  sensorVal += " CH4: ";
  sensorVal += MQ7.readSensor("CH4");
  sensorVal += " CO: ";
  sensorVal += MQ7.readSensor("CO");
  sensorVal += "Alcohol: ";
  sensorVal += MQ7.readSensor("Alcohol");

  duck.sendPayloadMessage(sensorVal);
  
  return true;
}