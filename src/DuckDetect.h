#ifndef DUCKDETECT_H
#define DUCKDETECT_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class DuckDetect : public Duck {
public:
  using Duck::Duck;

  ~DuckDetect() {}
  
  /**
   * @brief Send a ping message to devices in the mesh network.
   * 
   * @param startReceive `true` if the device must to be ready to receive a response immediately,
   * `false` if response needs to be deffered. 
   */
  void sendPing(bool startReceive);

  /**
   * @brief Provide the DuckDetect specific implementation of the base `run()`
   * method.
   *
   */
  void run();
  
  /**
   * @brief Override the default setup method to match DuckDetect specific
   * defaults.
   *
   * In addition to Serial component, the Radio component is also initialized.
   * When ssid and password are provided the duck will setup the wifi related
   * components.
   *
   * @param ssid wifi access point ssid (defaults to an empty string if not
   * provided)
   * @param password wifi password (defaults to an empty string if not provided)
   * 
   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
  int setupWithDefaults(std::vector<byte> deviceId, String ssid = "",
                        String password = "");

  /// callback definition for receiving RSSI value
  using rssiCallback = void (*)(const int);
  
  /**
   * @brief Regsiter a callback for receiving and handling RSSI value
   *
   * @param rssiCb a call back defined with the following signature: `void (*)(const int)`
   */
  void onReceiveRssi(rssiCallback rssiCb) { this->rssiCb = rssiCb; }

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  int getType() { return DuckType::DETECTOR; }

private:
  rssiCallback rssiCb;
  void handleReceivedPacket();
};
#endif