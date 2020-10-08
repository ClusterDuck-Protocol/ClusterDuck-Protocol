#ifndef PAPADUCK_H
#define PAPADUCK_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class PapaDuck : public Duck {
public:
  using Duck::Duck;
  
  /// Papa Duck callback function signature.
  using callbackFunc = void (*)(CDP_Packet );
  
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  void onReceiveDuckData(callbackFunc cb) { this->recvDataCallback = cb; }

  /**
   * @brief Provide the PapaDuck specific implementation of the base `run()`
   * method.
   *
   */
  void run();

  /**
   * @brief Override the default setup method to match the Duck specific
   * defaults.
   *
   * In addition to Serial component, the Radio component is also initialized.
   * When ssid and password are provided the duck will setup the wifi related
   * components.
   *
   * @param ssid wifi access point ssid (defaults to an empty string if not
   * provided)
   * @param password wifi password (defaults to an empty string if not provided)
   */
  void setupWithDefaults(String ssid = "", String password = "");

  /**
   * @brief Reconnect the device's WiFi access point.
   *
   * Allows a Wifi capable device to reconnect the wifi access point if it is
   * lost.
   *
   * @param ssid
   * @param password
   * @returns DUCK_ERR_NONE if the reconnection is successful, an error code
   * otherwise.
   */
  int reconnectWifi(String ssid, String password);

  
private:
  callbackFunc recvDataCallback;
  void handleReceivedPacket();
};

#endif