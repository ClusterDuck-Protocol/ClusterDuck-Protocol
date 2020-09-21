#ifndef DUCKLINK_H
#define DUCKLINK_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class DuckLink : public Duck {
public:
  using Duck::Duck;
  void run();
  void setupWithDefaults(String ssid="", String password="");
};

#endif