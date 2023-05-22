/**
 * @file WS2812Example.ino
 * @brief Uses the built in Mama Duck with WS2812 LEDs attached
 * 
 * This example is a Mama Duck, which also brings some light to the world by
 * incorporating some WS2812 LEDs via the FastLED library.
 *
 * @date 2020-11-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "arduino-timer.h"
#include <MamaDuck.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "FastLED.h"

// Setup for The W2812 LED
#define LED_PIN     4
#define NUM_LEDS    1
#define BRIGHTNESS  100
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

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
  duck.setupWithDefaults(devId);
  // Setup LED
  delay(3000); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("MAMA-DUCK...READY!");

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

  leds[0] = CRGB::Red;
  FastLED.show();
}

bool runSensor(void *) {
  return true;
}
