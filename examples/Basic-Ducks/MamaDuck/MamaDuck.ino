/**
 * @file MamaDuck.ino
 * @brief Implements a MamaDuck using the ClusterDuck Protocol (CDP).
 * 
 * This example firmware periodically sends sensor health data (counter and free memory) 
 * through a CDP mesh network. It also relays messages that it receives from other ducks
 * that has not seen yet.
 * 
 * @date 2025-05-07
 */

 #include <string>
 #include <arduino-timer.h>
 #include <CDP.h>

 #ifdef SERIAL_PORT_USBVIRTUAL
 #define Serial SERIAL_PORT_USBVIRTUAL
 #endif


// Setup for W2812 (LED)
#include <FastLED.h>
#include <pixeltypes.h>
#define LED_TYPE WS2812
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
CRGB leds[NUM_LEDS];
 
 // --- Function Declarations ---
 bool runSensor(void *);
 
 // --- Global Variables ---
 MamaDuck duck("MAMADUCK"); // Device ID, MUST be 8 bytes and unique from other ducks;
 auto timer = timer_create_default();  // Creating a timer with default settings
 const int INTERVAL_MS = 10000;        // Interval in milliseconds between runSensor call
 int counter = 1;                      // Counter for the sensor data  
 bool setupOK = false;                 // Flag to check if setup is complete
 
 /**
  * @brief Setup function to initialize the MamaDuck
  *
  * - Sets up the Duck device ID (exactly 8 bytes).
  * - Initializes MamaDuck using default configuration.
  * - Sets up periodic execution of sensor data transmissions.
  */
 void setup() {
  // Initialize LED
  FastLED.addLeds<LED_TYPE, CDPCFG_PIN_LED1, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(BRIGHTNESS);
  leds[0] = CRGB::Cyan;
  FastLED.show();
 
   if (duck.setupWithDefaults() != DUCK_ERR_NONE) {
      Serial.println("[MAMA] Failed to setup MamaDuck");
      leds[0] = CRGB::Red;
      FastLED.show();
      return;
    } else { 
      leds[0] = CRGB::Gold;
      FastLED.show();
    }
 
   timer.every(INTERVAL_MS, runSensor); // Triggers runSensor every INTERVAL_MS
   
   setupOK = true;
   Serial.println("[MAMA] Setup OK!");
 }
 
 /**
  * @brief Main loop runs continuously.
  *
  * Executes scheduled tasks and maintains Duck operation.
  */
 void loop() {
   if (!setupOK) {
     return; 
   }
   timer.tick();
 
   duck.run();
 }
 
 /**
  * @brief Periodically executed to gather and send health data.
  *
  * Collects the current counter value and available free memory, formats them
  * into a string, and transmits this data via CDP.
  *
  * @param unused Unused parameter required by the timer callback signature
  * @return true if data was successfully sent, false otherwise
  */
 bool runSensor(void *) {
   bool failure;
   
   std::string message = "C:" + std::to_string(counter) + "|" + "FM:" + std::to_string(freeMemory());
   Serial.print("[MAMA] sensor data: ");
   Serial.println(message.c_str());
 
   failure = duck.sendData(topics::health, message);
   if (!failure) {
     counter++;
     Serial.println("[MAMA] runSensor ok.");
   } else {
     Serial.println("[MAMA] runSensor failed.");
   }
   return true;
 }
 