#ifndef MAMADUCK_H
#define MAMADUCK_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class MamaDuck : public Duck {
public:
  using Duck::Duck;
  int run();
  void setup();
private:
  bool idInPath(String path);
};

#endif