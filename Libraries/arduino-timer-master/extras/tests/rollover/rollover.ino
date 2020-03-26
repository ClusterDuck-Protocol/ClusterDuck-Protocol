/*
 Test timer rollover handling
 */

#include <util/atomic.h>
#include <timer.h>

auto timer = timer_create_default();

// https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover
void set_millis(unsigned long ms)
{
    extern unsigned long timer0_millis;
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        timer0_millis = ms;
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    timer.every(1000, [](void *) -> bool {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        return true;
    });
}

void loop() {
    // 6-second time loop starting at rollover - 3 seconds
    if (millis() - (-3000) >= 6000)
        set_millis(-3000);
    timer.tick();
}
