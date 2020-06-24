#include <ClusterDuck.h>
//#include "timer.h"
//auto timer = timer_create_default(); // create a timer with default settings
#include <TinyGPS++.h>
#include <axp20x.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;
ClusterDuck duck;

void setup() {
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("X");
  duck.setupMamaDuck();
  //GPS Setup
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
  //// every 5 seconds run sensor
  // timer.every(5000, runSensor);
}

void loop() {
  // tick the timer
  // timer.tick();
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
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
  duck.sendPayloadStandard("gps", sensorVal);
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
  duck.sendPayloadStandard("gps", sensorVal);
  return true;
}