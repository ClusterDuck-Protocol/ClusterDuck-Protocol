/**
 * @file TBeam-GPS-Example.ino
 * @brief Uses the built in Mama Duck with some customizations.
 * 
 * This example is a Mama Duck, that has GPS capabilities and will send the GPS data with the GPS topic based on the set timer.
 * @date 2020-09-21
 * 
 * @copyright Copyright (c) 2020
 * ClusterDuck Protocol
 */

#include <string>
#include "arduino-timer.h"
#include <MamaDuck.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// GPS
#include <TinyGPS++.h>

// Power chip
#include <axp20x.h>

TinyGPSPlus tgps;
HardwareSerial GPS(1);
AXP20X_Class axp;

MamaDuck duck;

auto timer = timer_create_default();
const int INTERVAL_MS = 20000;
char message[32]; 
int counter = 1;

void setup() {
  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("MAMAGPS1");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId);
  Serial.println("MAMA-DUCK...READY!");

  // Setup AXP
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

bool runSensor(void *) {
  bool result;
  String sensorVal = getGPSData();

  Serial.print("[MAMA] sensor data: ");
  Serial.println(sensorVal);

  //Send gps data
  duck.sendData(topics::location, sensorVal);
  return true;
}
