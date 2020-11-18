/**
 * @file ClusterDuck.h
 * @author 
 * @brief This file defines the cluster duck public APIs.
 * 
 * @version 
 * @date 2020-09-15
 * 
 * @copyright
 * 
 */
#ifndef CD
#define CD

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <WString.h>

#include "DuckDisplay.h"
#include "include/DuckLed.h"
#include "include/DuckRadio.h"
#include "include/DuckNet.h"
#include "include/DuckUtils.h"
#include "include/cdpcfg.h"
#include "arduino-timer.h"

/**
 * @brief External APIs to build and control a duck device.
 * 
 * This class exposes all the necessary APIs to setup/build a duck as well as
 * customizig the duck's behavior.
 */
class ClusterDuck {
public:
  /**
   * @brief Construct a new Cluster Duck object.
   * 
   */
  ClusterDuck();

  ~ClusterDuck() {}
  

  /**
   * @brief Get the Duck Mac Address.
   *
   * @param format    true if the mac address is formated as MM:MM:MM:SS:SS:SS
   * @return A string representing the duck mac address
   */
  String duckMac(boolean format);

  /**
   * @brief Create a uuid string.
   *
   * @return A string representing a unique identifier.
   */
  String uuidCreator();

  /**
   * @brief Get the interrupt state.
   * 
   * @return true if interrupt is enabled, false otherwise.
   */
  volatile bool getInterrupt();

  /**
   * @brief Toggle the flag that indicates a message is received.
   * 
   */
  void flipFlag();

  /**
   * @brief Toggle the interrupt state flag.
   * 
   */
  void flipInterrupt();

  /**
   * @brief Transmit a ping message.
   *
   * @return 0 if the message was sent sucessfully, an error code otherwise.
   */
  int ping();

  /**
   * @brief Shortcut to setup a Duck Detector
   *
   * This function will setup a Duck Detector and is equivalent to call these
   * individual methods:
   * ```
   * setupDisplay("Duck");
   * setupRadio();
   * setupOTA()
   * ```
   * It is assumed that the Duck Detector hardware has a display and a wifi
   * component
   */
  void setupDetect();

  /**
   * @brief starts the Duck Detector run thread.
   *
   * - checks for "health" message received and returns the LoRa RSSI value
   * @return an integer value representing the LoRa RSSI value
   */
  int runDetect();

  /**
   * @brief Handles a received LoRa packet.
   *
   * @return The packet length which should be > 0. An error code is returned if there was a failure to read the data
   * from the LoRa driver.
   */
  int handlePacket();

  /**
   * @brief Get the Packet data.
   * 
   * @param pSize the length of the packet
   * @return A string representing the content of the packet 
   */
  String getPacketData(int pSize);


  /** 
   * @brief Get the last received LoRa packet.
   * 
   * @return A Packet object containing the last received message.
   */
  Packet getLastPacket();

  /**
   * @brief  Append a chunk to the packet.
   *
   * A chunk is part of the Duck LoRa packet and is formated as
   * [tag][length][payload] where a tag is a single byte identifying the chunk.
   * Currently supported tags:
   * ```
   * ping
   * sender_id
   * topic
   * message_id
   * payload
   * iamhere
   * ```
   * @param byteCode identifies the tag to append
   * @param outgoing the payload for the tag to be appended to the LoRa packet
   */
  void couple(byte byteCode, String outgoing);

  /**
   * @brief Determine if a Duck device_id is present in the path.
   *
   * @param path  path retrieved from the LoRa packet
   * @return true if the id is in the path, false otherwise.
   */
  bool idInPath(String path);   

  /**
   * @brief Get the packet receive flag status
   * 
   * @return true if a received packet is available, false otherwise. 
   */
  volatile bool getFlag();

  /**
   * @brief Set the Duck to be ready to recieve LoRa packets.
   *
   * @return 0 if the call was successful, an error code otherwise.
   */
  int startReceive();

  /**
   * @brief Set the Duck to be ready to transmit LoRa packets.
   *
   * @return 0 if the call was successful, an error code otherwise.
   */
  int startTransmit();

  /**
   * @brief Get the current RSSI value.
   *
   * @return An integer representing the rssi value.
   */
  int getRSSI();

  /**
   * @brief  Checks if the given ssid is available.
   *
   * @param val     ssid to check, default is an empty string and will use the
   * internal default ssid
   * @return true if the ssid is available, false otherwise.
   */
  bool ssidAvailable(String val = "");

  /**
   * @brief Set the WiFi network ssid.
   *
   * @param val the ssid string to set
   */
  void setSSID(String val);

  /**
   * @brief Set the WiFi password.
   *
   * @param val  the password string to set
   */
  void setPassword(String val);

  /**
   * @brief Get the WiFi network ssid.
   *
   * @return a string representing the current network ssid
   */
  String getSSID();

  /**
   * @brief Get the WiFi password ssid.
   *
   * @return a string representing the current network password
   */
  String getPassword();

  // OTA - Firmware upgrade
  void setupOTA();

  // Portal
  void processPortalRequest();
  String* getPortalDataArray();
  String getPortalDataString();
  bool runCaptivePortal();

protected:
  String _deviceId = "";

private:
  static void setFlag(void);

};
#endif