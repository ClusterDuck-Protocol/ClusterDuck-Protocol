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
   * @brief Enable or disable the sending of acknowledgements.
   *
   * The PapaDuck can send acknowledgements for each packet received. See
   * `reservedTopic::ack`. By default, the PapaDuck will not send acks.
   *
   * @param enable `true` if PapaDuck should send acks, `false` if not.
   */
  void enableAcks(bool enable);

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

  /**
   * @brief Either store a DUID/MUID pair for later or send an ack broadcast.
   *
   * Also can enable the timer if necessary.
   *
   * @param packet The recently received packet.
   */
  void handleAck(const CdpPacket & packet);

  /**
   * @brief Indicates whether the packet should be acked or not.
   *
   * @param packet A valid, received CDP packet
   * @returns True if so, false if not
   */
  bool needsAck(const CdpPacket & packet);

  /**
   * @brief Stores information from packet such that it will be acked later.
   *
   * @param packet A valid, received CDP packet
   */
  void storeForAck(const CdpPacket & packet);

  /**
   * @brief Indicates whether the ack buffer is full.
   *
   * @returns True if true, false if not
   */
  bool ackBufferIsFull();

  /**
   * @brief Sends a broadcast ack, containing one or more DUID/MUIDs to ack.
   */
  void broadcastAck();

  static bool ackHandler(PapaDuck * duck);

  typedef std::vector<std::pair<Duid, Muid> > AckStore;

  bool acksEnabled{false};
  Timer<1, millis, PapaDuck*> ackTimer;
  size_t timerDelay{1000}; // TODO(rolsen): Set to something more reasonable.
  AckStore ackStore;
  rxDoneCallback recvDataCallback;
};

#endif
