# arduino-timer - library for delaying function calls

Simple *non-blocking* timer library for calling functions **in / at / every** specified units of time. Supports millis, micros, time rollover, and compile time configurable number of tasks.

### Use It

Include the library and create a *Timer* instance.
```cpp
#include <timer.h>

auto timer = timer_create_default();
```

Or using the *Timer* constructors for different task limits / time resolution
```cpp
Timer<10> timer; // 10 concurrent tasks, using millis as resolution
Timer<10, micros> timer; // 10 concurrent tasks, using micros as resolution
```

Call *timer*.**tick()** in the loop function
```cpp
void loop() {
    timer.tick();
}
```

Make a function to call when the *Timer* expires
```cpp
bool function_to_call(void *argument /* optional argument given to in/at/every */) {
    return true; // to repeat the action - false to stop
}
```

Call *function\_to\_call* **in** *delay* units of time *(unit of time defaults to milliseconds)*.
```cpp
timer.in(delay, function_to_call);
timer.in(delay, function_to_call, argument); // or with an optional argument for function_to_call
```

Call *function\_to\_call* **at** a specific *time*.
```cpp
timer.at(time, function_to_call);
timer.at(time, function_to_call, argument); // with argument
```

Call *function\_to\_call* **every** *interval* units of time.
```cpp
timer.every(interval, function_to_call);
timer.every(interval, function_to_call, argument); // with argument
```

Be fancy with **lambdas**
```cpp
timer.in(1000, [](void*) -> bool { return false; });
timer.in(1000, [](void *argument) -> bool { return argument; }, argument);
```

### API

```cpp
/* Constructors */
/* Create a timer object with default settings - millis resolution, TIMER_MAX_TASKS (=16) task slots */
Timer<> timer_create_default(); // auto timer = timer_create_default();

/* Create a timer with max_tasks slots and time_func resolution */
Timer<size_t max_tasks = TIMER_MAX_TASKS, unsigned long (*time_func)(void) = millis> timer;
Timer<> timer; // Equivalent to: auto timer = timer_create_default()
Timer<10> timer; // Timer with 10 task slots
Timer<10, micros> timer; // timer with 10 task slots and microsecond resolution

/* Signature for handler functions */
bool handler(void *argument);

/* Timer Methods */
/* Ticks the timer forward - call this function in loop() */
void tick();

/* Calls handler with opaque as argument in delay units of time */
bool in(unsigned long delay, handler_t handler, void *opaque = NULL);

/* Calls handler with opaque as argument at time */
bool at(unsigned long time, handler_t handler, void *opaque = NULL);

/* Calls handler with opaque as argument every interval units of time */
bool every(unsigned long interval, handler_t handler, void *opaque = NULL);
```

### Installation

[Check out the instructions](https://www.arduino.cc/en/Guide/Libraries) from Arduino.

**OR** copy **src/timer.h** into your project folder *(you won't get managed updates this way)*.

### Examples

Found in the **examples/** folder.

The simplest example, blinking an LED every second *(from examples/blink)*:

```cpp
#include <timer.h>

auto timer = timer_create_default(); // create a timer with default settings

bool toggle_led(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // keep timer active? true
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // set LED pin to OUTPUT

  // call the toggle_led function every 1000 millis (1 second)
  timer.every(1000, toggle_led);
}

void loop() {
  timer.tick(); // tick the timer
}
```

### LICENSE

Check the LICENSE file - 3-Clause BSD License

### Notes

Currently only a software timer. Any blocking code delaying *timer*.**tick()** will prevent the timer from moving forward and calling any functions.

The library does not do any dynamic memory allocation.

The number of concurrent tasks is a compile time constant, meaning there is a limit to the number of concurrent tasks. The **in / at / every** functions return **false** if the *Timer* is full.

Change the number of concurrent tasks using the *Timer* constructors. Save memory by reducing the number, increase memory use by having more. The default is **TIMER_MAX_TASKS** which is currently 16.
