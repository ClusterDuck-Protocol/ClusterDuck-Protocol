#ifndef DUCKDETECT_H
#define DUCKDETECT_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class DuckDetect : public Duck {
public:
  using Duck::Duck;
  int run();
  int startReceive();
  int startTransmit();
  void setup();
};
#endif