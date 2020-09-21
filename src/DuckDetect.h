#ifndef DUCKDETECT_H
#define DUCKDETECT_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class DuckDetect : public Duck {
public:
  using Duck::Duck;
  void run();
  void sendPing(bool startReceive);
  void setupWithDefaults(String ssid = "", String password = "");

  using rssiCallback = void (*)(const int);
  void onReceiveRssi(rssiCallback rssiCb) { this->rssiCb = rssiCb; }

private:
  rssiCallback rssiCb;
};
#endif