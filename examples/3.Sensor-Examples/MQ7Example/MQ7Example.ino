/**
 * @file mamaduck-send-message.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * This example is a Mama Duck, but it is also periodically sending a message in the Mesh
 * It is setup to provide a custom Emergency portal, instead of using the one provided by the SDK.
 * Notice the background color of the captive portal is Black instead of the default Red.
 * 
 * @date 2020-09-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <MamaDuck.h>
#include <arduino-timer.h>
#include <string>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif


//Include the MQ7 library
#include <MQUnifiedsensor.h>

//Definitions
#define pin A0 //Analog input 0 of your arduino
#define type 7 //MQ7
//#define calibration_button 13 //Pin to calibrate your sensor

//Declare Sensor
MQUnifiedsensor MQ7(pin, type);


// create a built-in mama duck
MamaDuck duck = MamaDuck();

auto timer = timer_create_default();
const int INTERVAL_MS = 60000;
char message[32]; 
int counter = 1;

void setup() {
  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("MAMA0001");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId);
  Serial.println("MAMA-DUCK...READY!");

  // init the sensor
  /*****************************  MQInicializar****************************************
  Input:  pin, type 
  Output:  
  Remarks: This function create the sensor object.
  ************************************************************************************/
  MQ7.init();
  //pinMode(calibration_button, INPUT);

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

  duck.sendData(topics::sensor, sensorVal);
  return true;
}