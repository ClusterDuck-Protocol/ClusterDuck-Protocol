/**
 * @file MQ7Example.ino
 * @brief Uses the built in Mama Duck with a MQ7 sensor attached
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
#define placa "Heltec WiFi LoRa 32(V2)"
#define Voltage_Resolution 5
#define pin A0 //Analog input 0 of your arduino
#define type "MQ-7" //MQ7
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO
#define RatioMQ7CleanAir 27.5 //RS / R0 = 27.5 ppm 
#define PWMPin 5 // Pin connected to mosfet

//Declare Sensor
MQUnifiedsensor MQ7(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);


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


   //Set math model to calculate the PPM concentration and the value of constants
  MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ7.setA(99.042); MQ7.setB(-1.518); // Configurate the ecuation values to get CO concentration

  // init the sensor
  /*****************************  MQInicializar****************************************
  Input:  pin, type 
  Output:  
  Remarks: This function create the sensor object.
  ************************************************************************************/
  MQ7.init();
    /*****************************  MQ CAlibration ********************************************/ 
  // Explanation: 
  // In this routine the sensor will measure the resistance of the sensor supposing before was pre-heated
  // and now is on clean air (Calibration conditions), and it will setup R0 value.
  // We recomend execute this routine only on setup or on the laboratory and save on the eeprom of your arduino
  // This routine not need to execute to every restart, you can load your R0 if you know the value
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ7.update(); // Update data, the arduino will be read the voltage on the analog pin
    calcR0 += MQ7.calibrate(RatioMQ7CleanAir);
    Serial.print(".");
  }
  MQ7.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  MQ7.serialDebug(true);
  

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

 
    MQ7.update(); // Update data, the arduino will be read the voltage on the analog pin
    MQ7.readSensor(); // Sensor will read PPM concentration using the model and a and b values setted before or in the setup
    MQ7.serialDebug(); // Will print the table on the serial port

    float sensorRead = MQ7.readSensor(); 
    String sensorVal = String(sensorRead);

 

  duck.sendData(topics::sensor, sensorVal);
  return true;
}