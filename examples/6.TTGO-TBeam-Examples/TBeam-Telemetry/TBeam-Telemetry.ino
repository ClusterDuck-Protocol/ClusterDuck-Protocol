/**
 * @file TBeam-Telemetry.ino
 * @brief Uses the built in Mama Duck and reports additional telemetry data.
 * 
 * This example is a Mama Duck, that has GPS capabilities and will send telemetry data with the GPS topic based on the set timer.
 * @date 2021-04-08
 * 
 * @copyright Copyright (c) 2021
 * ClusterDuck Protocol
 */

#include <string>
#include <arduino-timer.h>
#include <MamaDuck.h>
#include <MemoryFree.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif


//GPS
#include <TinyGPS++.h>
#include <axp20x.h>

TinyGPSPlus tgps;
HardwareSerial GPS(1);

// AXP setup (Power)
#include <Wire.h>
#include <axp20x.h>
AXP20X_Class axp;

MamaDuck duck;

auto timer = timer_create_default();
const int INTERVAL_MS = 10000;
char message[32]; 
int counter = 1;

void setup() {
  // The Device ID must be unique and 8 bytes long. Typically the ID is stored
  // in a secure nvram, or provided to the duck during provisioning/registration
  std::string deviceId("MAMADUCK");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId);

  Serial.println("MAMA-DUCK...READY!");
   Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  GPS.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX

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


static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      tgps.encode(GPS.read());
  } while (millis() - start < ms);
}

bool runSensor(void *) {
    
  // Creating a boolean to store the result
  bool result;
  
  // Creating a buffer to save message
  const byte* buffer;

  // Creating the message to be sent 
  String message = "Counter:" + String(counter)+ " " +String("Free Memory:") + String(freeMemory()) + " " + getGPSData() + " " + getBatteryData();
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message);
  
  // Converting the message from string tp bytes
  int length = message.length();
  buffer = (byte*) message.c_str(); 
  
  // Sending the data
  result = sendData(buffer, length);
  if (result) {
     Serial.println("[MAMA] runSensor ok.");
  } else {
     Serial.println("[MAMA] runSensor failed.");
  }
  // Return result
  return result;
}

// Getting GPS data
String getGPSData() {

  // Encoding the GPS
  smartDelay(5000);
  
  // Printing the GPS data
  Serial.println("--- GPS ---");
  Serial.print("Latitude  : ");
  Serial.println(tgps.location.lat(), 5);  
  Serial.print("Longitude : ");
  Serial.println(tgps.location.lng(), 4);
  Serial.print("Altitude  : ");
  Serial.print(tgps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Satellites: ");
  Serial.println(tgps.satellites.value());
  Serial.print("Time      : ");
  Serial.print(tgps.time.hour());
  Serial.print(":");
  Serial.print(tgps.time.minute());
  Serial.print(":");
  Serial.println(tgps.time.second());
  Serial.print("Speed     : ");
  Serial.println(tgps.speed.kmph());
  Serial.println("**********************");
  
  // Creating a message of the Latitude and Longitude
  String sensorVal = "Lat:" + String(tgps.location.lat(), 5) + " Lng:" + String(tgps.location.lng(), 4) + " Alt:" + String(tgps.altitude.feet() / 3.2808);

  // Check to see if GPS data is being received
  if (millis() > 5000 && tgps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS data received: check wiring"));
  }

  return sensorVal;
}

bool sendData(const byte* buffer, int length) {
  bool sentOk = false;
  
  // Send Data can either take a byte buffer (unsigned char) or a vector
  int err = duck.sendData(topics::location, buffer, length);
  if (err == DUCK_ERR_NONE) {
     counter++;
     sentOk = true;
  }
  if (!sentOk) {
    Serial.println("[MAMA] Failed to send data. error = " + String(err));
  }
  return sentOk;
}

// Getting the battery data
String getBatteryData() {
  
  int isCharging = axp.isChargeing();
  boolean isFullyCharged = axp.isChargingDoneIRQ();
  float batteryVoltage = axp.getBattVoltage();
  float batteryDischarge = axp.getAcinCurrent();
  float getTemp = axp.getTemp();  
  float battPercentage = axp.getBattPercentage();
   
  Serial.println("--- Power ---");
  Serial.print("Duck charging (1 = Yes): ");
  Serial.println(isCharging);
  Serial.print("Fully Charged: ");
  Serial.println(isFullyCharged);
  Serial.print("Battery Voltage: ");
  Serial.println(batteryVoltage);
  Serial.print("Battery Discharge: ");
  Serial.println(batteryDischarge);  
  Serial.print("Board Temperature: ");
  Serial.println(getTemp);
  Serial.print("battery Percentage: ");
  Serial.println(battPercentage);
   

  String sensorVal = 
  "Charging:" + 
  String(isCharging) +  
  " Full:" +
  String(isFullyCharged)+
  " Volts:" +
  String(batteryVoltage) + 
  " Temp:" +
  String(getTemp);

  return sensorVal;
}