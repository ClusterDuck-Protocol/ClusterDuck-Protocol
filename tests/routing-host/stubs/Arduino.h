#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
using byte = uint8_t;
using boolean = bool;
inline uint32_t hostMillis = 0;
inline uint32_t hostRandom = 7;
inline unsigned long millis() { return hostMillis; }
inline unsigned long micros() { return hostMillis * 1000UL; }
inline uint32_t esp_random() { hostRandom = hostRandom * 1664525U + 1013904223U; return hostRandom; }
inline long random(long n) { return esp_random() % n; }
inline void delay(unsigned long ms) { hostMillis += ms; }
struct HostSerial {
    void begin(int) {}
    template<class... T> int printf(const char*, T...) { return 0; }
    template<class T> void println(const T&) {}
    template<class T> void print(const T&) {}
    void println() {}
};
inline HostSerial Serial;
