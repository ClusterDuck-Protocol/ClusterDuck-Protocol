/**
 * @file TTGO-APX-Example.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * This example is a Mama Duck for the TTGOm that provides feedback adn status on the battery and charginf of the duck.
 * 
 * @date 2020-11-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "timer.h"
#include <MamaDuck.h>
#include <DuckDisplay.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// APX setup
#include <Wire.h>
#include <axp20x.h>

AXP20X_Class axp;

const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;


DuckDisplay* display = NULL;


// Set device ID between ""
String deviceId = "MAMA001";
MamaDuck duck = MamaDuck(deviceId);

auto timer = timer_create_default();
const int INTERVAL_MS = 60000;
char message[32]; 
int counter = 1;

void setup() {
  // Use the default setup provided by the SDK
  duck.setupWithDefaults();
  Serial.println("MAMA-DUCK...READY!");
  // initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);

   Wire.begin(i2c_sda, i2c_scl);

     int ret = axp.begin(Wire, AXP192_SLAVE_ADDRESS);

     if (ret == AXP_FAIL) {
        Serial.println("AXP Power begin failed");
        while (1);
     }
}

void loop() {
  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();


}

bool runSensor(void *) {
  
  
float isCharging = axp.isChargeing();
boolean isFullyCharged = axp.isChargingDoneIRQ();
float batteryVoltage = axp.getBattVoltage();
float batteryDischarge = axp.getAcinCurrent();
float getTemp = axp.getTemp();  
int battPercentage = axp.getBattPercentage();
   
    Serial.println("--- T-BEAM Power Information ---");
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
  "Charging: " + 
  String(isCharging) ; 
  " BattFull: " +
  String(isFullyCharged)+
  " Voltage " +
  String(batteryVoltage) ;
  " Temp: " +
  String(getTemp);

  
  duck.sendPayloadStandard(sensorVal, "power");
  return true;
}
