/*
 * timer_blink_print
 *
 * Blinks the built-in LED every half second, and prints a messages every
 * second using the arduino-timer library.
 *
 */

#include <timer.h>

auto timer = timer_create_default(); // create a timer with default settings

bool toggle_led(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // repeat? true
}

bool print_message(void *) {
  Serial.print("print_message: Called at: ");
  Serial.println(millis());
  return true; // repeat? true
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); // set LED pin to OUTPUT

  // call the toggle_led function every 500 millis (half second)
  timer.every(500, toggle_led);

  // call the print_message function every 1000 millis (1 second)
  timer.every(1000, print_message);
}

void loop() {
  timer.tick(); // tick the timer
}
