#pragma once
#include <Arduino.h>
#include <deque>
#include <functional>
#include <optional>
#include <utility>
#define DUCKLOGGER_H
#define loginfo_ln(...) ((void)0)
#define loginfo(...) ((void)0)
#define logerr_ln(...) ((void)0)
#define logerr(...) ((void)0)
#define logdbg_ln(...) ((void)0)
#define logdbg(...) ((void)0)
#define logwarn_ln(...) ((void)0)
#define logwarn(...) ((void)0)
#define DUCKLORA_H_
#define DUCKWIFI_H
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_PROGMEM 0
struct LoRaConfigParams { float band; int8_t txPower; float bw; uint8_t sf, gain; void (*func)(); };
class DuckLoRa;
#include "wifi/DuckWifiNone.h"
using DuckWifi = DuckWifiNone;
namespace duckesp {
inline int getMinFreeHeap() { return 100000; }
inline int getMaxAllocHeap() { return 100000; }
inline void restartDuck() {}
}
inline int freeMemory() { return 100000; }
