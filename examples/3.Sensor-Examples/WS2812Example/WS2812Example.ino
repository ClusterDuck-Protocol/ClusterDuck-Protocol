/**
 * @file Led-Example.ino
 * @brief Uses the built in Mama Duck with some customatizations.
 * 
 * This example is a Mama Duck, but it is also periodically sending a message in the Mesh
 * It is setup to provide a custom Emergency portal, instead of using the one provided by the SDK.
 * Notice the background color of the captive portal is Black instead of the default Red.
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

#include "FastLED.h"

// Setup for The W2812 LED
#define LED_TYPE NEOPIXEL   // See FastLED library for all the different leds supprt 
#define DATA_PIN 4         // Pin on the board for connecting data in for LED
#define NUM_LEDS 1        // Nuber of LEDS on your Led Strip (use 1 for single LED)
CRGB leds[NUM_LEDS];  
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

  display = DuckDisplay::getInstance();
  display->setupDisplay(duck.getType(), deviceId );
  // we are done
  display->showDefaultScreen();

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
