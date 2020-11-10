/**
 * @file DetectorDuck.ino
 * @brief Builds a Duck to get RSSI signal strength value.
 * 
 * This example builds a duck using the preset DuckDetect to periodically send a ping message
 * then provide the RSSI value of the response.
 * 
 * @date 2020-11-10
 *
 * @copyright Copyright (c) 2020
 * ClusterDuck Protocol 
 */

#include <DuckDetect.h>
#include "timer.h"

// Needed if using a board with built-in USB, such as Arduiono Zero
#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// We use the built-in DetectorDuck with a given Device UID
DuckDetect duck = DuckDetect("DUCK-DETECTOR");

// Create a timer with default settings
auto timer = timer_create_default(); 

void setup() {

   duck.setupWithDefaults();

   Serial.println("DUCK-DETECTOR...READY!");

   // Register  a callback that provides RSSI value
   duck.onReceiveRssi(handleReceiveRssi);
   
   timer.every(30000, ping);
}

void handleReceiveRssi(const int rssi) {
   Serial.println("[DUCK-DETECTOR] handleReceiveRssi()");
   showSignalQuality(rssi);
}

void loop() {
   timer.tick();
   duck.run(); // use internal duck detect behavior
}

// Periodically sends a ping message
bool ping(void *) {
   Serial.println("[DUCK-DETECTOR] Says ping!");
   // This API is only available to Duck Detector
   duck.sendPing(true);

   return true;
}

// This uses the serial console to output the RSSI quality
// But you can use a display, sound or LEDs
void showSignalQuality(int incoming) {
   int rssi = incoming;   
   Serial.print("[DUCK-DETECTOR] Rssi value: ");
   Serial.print(rssi);

   if(rssi > -95) {
      Serial.println(" - GOOD");
   }
   else if(rssi <= -95 && rssi > -108) {
      Serial.println(" - OKAY");
   }
   else if(rssi <= -108) {
      Serial.println(" - BAD");
   }
}