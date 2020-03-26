/*
 * timer_blink_micros
 *
 * Blinks the built-in LED every second using the arduino-timer library.
 *
 */

#include <timer.h>

Timer<1, micros> timer; // create a timer with 1 task and microsecond resolution

bool toggle_led(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // repeat? true
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // set LED pin to OUTPUT

  // call the toggle_led function every 1000000 micros (1 second)
  timer.every(1000000, toggle_led);
}

void loop() {
  timer.tick(); // tick the timer
}
