/**
 * @file TBeam-AXP-Example.ino
 * @brief Uses the built in Mama Duck with some customizations.
 * 
 * This example is a Mama Duck for the TTGO T-Beam that provides feedback and status on the battery and charging of the duck.
 * 
 * @date 2020-11-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <string>
#include "arduino-timer.h"
#include <MamaDuck.h>
#include <DuckDisplay.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// AXP setup
#include <Wire.h>
#define XPOWERS_CHIP_AXP192
#include <XPowersLib.h>

XPowersPMU axp;

bool runSensor(void *);
const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;


DuckDisplay* display = NULL;


// Set device ID between ""
MamaDuck duck;

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

  // initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);

   Wire.begin(i2c_sda, i2c_scl);

     int ret = axp.begin(Wire, AXP192_SLAVE_ADDRESS,i2c_sda,i2c_scl);

     axp.enableIRQ(XPOWERS_AXP192_BAT_CHG_DONE_IRQ | XPOWERS_AXP192_BAT_CHG_START_IRQ);
}

void loop() {
  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();


}

bool runSensor(void *) {
  
  
float isCharging = axp.isCharging();
boolean isFullyCharged = axp.isBatChagerDoneIrq();
float batteryVoltage = axp.getBattVoltage();
float batteryDischarge = axp.getAcinCurrent();
float getTemp = axp.getTemperature();
int battPercentage = axp.getBatteryPercent();
   
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


  std::string sensorVal =
  "Charging: ";
  sensorVal.append(isCharging ? "Yes" : "No")
  .append(" BattFull: ")
  .append(isFullyCharged ? "Yes" : "No")
  .append(" Voltage: ")
  .append(std::to_string(batteryVoltage))
  .append(" Temp: ")
  .append(std::to_string(getTemp));

  
  duck.sendData(topics::sensor, sensorVal);
  return true;
}
