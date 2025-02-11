#ifndef PAPADUCK_H
#define PAPADUCK_H

#include <Arduino.h>
#include <arduino-timer.h>
#include "include/Duck.h"

class PapaDuck : public Duck {
public:
  using Duck::Duck;
  
  ~PapaDuck() {}

  /// Papa Duck callback functions signature.
  using rxDoneCallback = void (*)(std::vector<byte> data );
  using txDoneCallback = void (*)(void);
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

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
   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
   int setupWithDefaults(std::array<byte,8> deviceId, std::string ssid = "",
                            std::string password = "");

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
  int reconnectWifi(std::string ssid, std::string password);

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  int getType() { return DuckType::PAPA; }

  /**
   * @brief Send duck command to entire network
   *
   * The PapaDuck can send command messages to remote ducks. There are
   * two parts for each command. Only PapaDucks are able to send 
   * commands to remote ducks. See `MamaDuck.cpp` for available commands.
   * All ducks will execute this command and relay.
   *
   * @param cmd byte enum for command to be executed.
   * @param value contextual data to be used in executed command.
   */
  void sendCommand(byte cmd, std::vector<byte> value);

  /**
   * @brief Send duck command to specific duck
   *
   * The PapaDuck can send command messages to remote ducks. There are
   * two parts for each command. Only PapaDucks are able to send 
   * commands to remote ducks. See `MamaDuck.cpp` for available commands. 
   *
   * @param cmd byte enum for command to be executed.
   * @param value contextual data to be used in executed command.
   * @param dduid destination duck ID for command to be executed.
   */
  void sendCommand(byte cmd, std::vector<byte> value, std::array<byte,8> dduid);

private:

  void handleReceivedPacket();

  rxDoneCallback recvDataCallback;
};

#endif
