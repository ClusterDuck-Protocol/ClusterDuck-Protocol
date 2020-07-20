#include <ClusterDuck.h>
//#include "timer.h"
#include "FastLED.h"

// Setup for The W2812 LED
#define LED_TYPE NEOPIXEL   // See FastLED library for all the different leds supprt 
#define DATA_PIN 4         // Pin on the board for connecting data in for LED
#define NUM_LEDS 1        // Nuber of LEDS on your Led Strip (use 1 for single LED)
CRGB leds[NUM_LEDS];  

//auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;

void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("BETA");
  duck.setupMamaDuck();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // Setup LED
  //timer.every(300000, runSensor);
}

void loop() {
  //timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  leds[0] = CRGB::Red;
  FastLED.show();
  
  
}

bool runSensor(void *) {

  return true;
}
