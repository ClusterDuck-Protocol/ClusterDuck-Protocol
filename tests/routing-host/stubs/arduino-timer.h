#pragma once
#include <Arduino.h>
#include <vector>
template<size_t N> class Timer {
    struct Task { uint32_t interval, last; bool (*fn)(void*); void* arg; bool active; };
    std::vector<Task> tasks;
public:
    void every(uint32_t interval, bool (*fn)(void*), void* arg) {
        tasks.push_back({interval, uint32_t(millis()), fn, arg, true});
    }
    void tick() {
        for (auto& t : tasks) if (t.active && uint32_t(millis() - t.last) >= t.interval) {
            t.last = millis(); t.active = t.fn(t.arg);
        }
    }
};
