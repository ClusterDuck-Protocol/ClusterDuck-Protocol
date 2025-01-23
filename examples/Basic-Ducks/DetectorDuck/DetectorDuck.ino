/**
   @file DetectorDuck.ino
   @author
   @brief Builds a Duck to get RSSI signal strength value.

   This example builds a duck using the preset DuckDetect to periodically send a ping message
   then provide the RSSI value of the response.

   @version
   @date 2020-09-21

   @copyright
*/

#include <arduino-timer.h>
#include <string>
#include <DuckDetect.h>
#include "FastLED.h"

// Setup for W2812 (LED)
#define LED_TYPE WS2812
#define DATA_PIN 4
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
#include <pixeltypes.h>
CRGB leds[NUM_LEDS]; 

// Needed if using a board with built-in USB, such as Arduiono Zero
#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create an instance of a built-in Duck Detector
DuckDetect duck;

// Create a timer with default settings
auto timer = timer_create_default();

const int INTERVAL_MS = 5000;
const unsigned long SIGNAL_TIMEOUT_MS = INTERVAL_MS + 1000; // 5 seconds timeout
unsigned long lastSignalTime = 0;

void setup() {

  std::string deviceId("DETECTOR"); // NOTE: The Device ID must be exactly 8 bytes
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  duck.setupWithDefaults(devId);

  // Register  a callback that provides RSSI value
  duck.onReceiveRssi(handleReceiveRssi);

  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, pingHandler);

  // Setup the LED
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(  BRIGHTNESS );
  leds[0] = CRGB::Gold;
  FastLED.show();

  Serial.println("[DETECTOR] Setup OK!");
}

void handleReceiveRssi(const int rssi) {
  Serial.println("[DETECTOR] RSSI callback called");
  showSignalQuality(rssi);
  Serial.println("[DETECTOR] Reseting signal timeout");
  lastSignalTime = millis();
}

void loop() {
  timer.tick();
  duck.run(); // use internal duck detect behavior
  
  // Check if signal timeout occurred
  if (millis() - lastSignalTime > SIGNAL_TIMEOUT_MS) {
    Serial.println("[DETECTOR] No signal");
    leds[0] = CRGB::Red;
    FastLED.show();
    lastSignalTime = millis(); // Reset to prevent continuous messages
  }
}

// Periodically sends a ping message
bool pingHandler(void *) {
  duck.sendPing();

  return true;
}

// This uses the serial console to output the RSSI quality
// But you can use a display, sound or LEDs
void showSignalQuality(int incoming) {
  int rssi = incoming;
  Serial.print("[DETECTOR] RSSI value: ");
  Serial.print(rssi);

  if (rssi > -95) {
    Serial.println(" - GOOD SIGNAL");
    leds[0] = CRGB::Green;
    FastLED.show();
  }
  else if (rssi <= -95 && rssi > -108) {
    Serial.println(" - OKAY SIGNAL");
    leds[0] = CRGB::Blue;
    FastLED.show();
  }
  else if (rssi <= -108 <= -118) {
    Serial.println(" - LOW SIGNAL");
    leds[0] = CRGB::Purple;
    FastLED.show();
  }
  else {
    Serial.println(" - NO SIGNAL");
    leds[0] = CRGB::Red;
    FastLED.show();
  }
}