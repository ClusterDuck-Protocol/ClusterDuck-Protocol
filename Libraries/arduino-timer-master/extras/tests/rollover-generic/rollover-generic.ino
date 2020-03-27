/*
 Test timer rollover handling
 */

#include <timer.h>

unsigned long wrapping_millis();

Timer<1, wrapping_millis> timer; // this timer will wrap
auto _timer = timer_create_default(); // to count milliseconds

unsigned long _millis = 0L;
unsigned long wrapping_millis()
{
    // uses _millis controled by _timer
    // 6-second time loop starting at rollover - 3 seconds
    if (_millis - (-3000) >= 6000)
        _millis = -3000;
    return _millis;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    _timer.every(1, [](void *) -> bool {
        ++_millis; // increase _millis every millisecond
        return true;
    });

    // should blink the led every second, regardless of wrapping
    timer.every(1000, [](void *) -> bool {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        return true;
    });
}

void loop() {
    _timer.tick();
    timer.tick();
}
