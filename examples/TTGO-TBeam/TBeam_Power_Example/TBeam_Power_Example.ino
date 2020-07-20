#include <ClusterDuck.h>
//#include "timer.h"

#include <Wire.h>
#include <axp20x.h>

AXP20X_Class axp;

const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;

auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("PowerTest");
  duck.setupMamaDuck();


  Wire.begin(i2c_sda, i2c_scl);

     int ret = axp.begin(Wire, AXP192_SLAVE_ADDRESS);

     if (ret == AXP_FAIL) {
        Serial.println("AXP Power begin failed");
        while (1);
     }

  timer.every(10000, runSensor);
}

void loop() {
  timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
  
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
//  " BattFull: " +
//  String(isFullyCharged)+
//  " Voltage " +
//  String(batteryVoltage) ;
//  " Temp: " +
//  String(getTemp);

  
  duck.sendPayloadStandard(sensorVal, "power");
  return true;
}
