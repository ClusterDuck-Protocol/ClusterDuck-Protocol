/*
 * timer_blink
 *
 * Blinks the built-in LED every second using the arduino-timer library.
 *
 */

#include <timer.h>

auto timer = timer_create_default(); // create a timer with default settings

bool toggle_led(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // repeat? true
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // set LED pin to OUTPUT

  // call the toggle_led function every 1000 millis (1 second)
  timer.every(1000, toggle_led);
}

void loop() {
  timer.tick(); // tick the timer
}
