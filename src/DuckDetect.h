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
  void sendPing(bool startReceive);
  void setupWithDefaults();

  using MessageRxCallback = void (*)(const byte*);
  void registerRecvCallback(MessageRxCallback rxCb) { this->rxCb = rxCb; }

private:
  MessageRxCallback rxCb;
};
#endif