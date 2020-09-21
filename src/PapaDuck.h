#ifndef PAPADUCK_H
#define PAPADUCK_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class PapaDuck : public Duck {
public:
  using Duck::Duck;
  
  /**
   * @brief Papa Duck callback function signature.
   * 
   */
  using callbackFunc = void (*)(const Packet);
  
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * @param cb 
   */
  void onReceiveDuckData(callbackFunc cb) { this->recvDataCallback = cb; }

  void run();
  void setupWithDefaults();
  int reconnectWifi(String ssid, String password);

private:
  callbackFunc recvDataCallback;
};

#endif