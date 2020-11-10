/**
 * @file TTGOGPSExample.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * This example is a Mama Duck, that has GPS capabilities and will send the GPS data with the GPS topic based on the set timer.
 * @date 2020-09-21
 * 
 * @copyright Copyright (c) 2020
 * ClusterDuck Protocol
 */

#include "timer.h"
#include <MamaDuck.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif


//GPS
#include <TinyGPS++.h>
#include <axp20x.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;


MamaDuck duck = MamaDuck("BETA");

auto timer = timer_create_default();
const int INTERVAL_MS = 20000;
char message[32]; 
int counter = 1;

void setup() {
  // Use the default setup provided by the SDK
  duck.setupWithDefaults();
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
  Serial.print("Latitude  : ");
  Serial.println(gps.location.lat(), 5);
  Serial.print("Longitude : ");
  Serial.println(gps.location.lng(), 4);
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("Altitude  : ");
  Serial.print(gps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Time      : ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  Serial.print("Speed     : ");
  Serial.println(gps.speed.kmph()); 
  Serial.println("**********************");
  String sensorVal = "Lat: " + String(gps.location.lat(), 5) + " Lng: " + String(gps.location.lng(), 4);
  Serial.println(" ");
  Serial.println(sensorVal);
  Serial.println(" ");
  duck.sendPayloadStandard(sensorVal, "gps");
  smartDelay(7500);
  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}


static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      gps.encode(GPS.read());
  } while (millis() - start < ms);
}

bool runSensor(void *) {
  Serial.print("Latitude  : ");
  Serial.println(gps.location.lat(), 5);  
  Serial.print("Longitude : ");
  Serial.println(gps.location.lng(), 4);
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("Altitude  : ");
  Serial.print(gps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Time      : ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  Serial.print("Speed     : ");
  Serial.println(gps.speed.kmph());
  Serial.println("**********************");
  
  String sensorVal = "Lat: " + String(gps.location.lat(), 5) + " Lng: " + String(gps.location.lng(), 4);
  
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS data received: check wiring"));
  }
  Serial.println(" ");
  Serial.println(sensorVal);
  duck.sendPayloadStandard(sensorVal, "gps");
  return true;
}