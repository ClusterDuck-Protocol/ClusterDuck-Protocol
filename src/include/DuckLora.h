/**
 * @file DuckLora.h
 * @brief This file is internal to CDP and provides the library access to
 * onboard LoRa module functions as well as packet management.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */

#ifndef DUCKLORA_H_
#define DUCKLORA_H_

#include <Arduino.h>
#include <RadioLib.h>

#include "../DuckError.h"

#include "DuckPacket.h"
#include "LoraPacket.h"
#include "cdpcfg.h"

/**
 * @brief Internal structure to hold the LoRa module configuration
 * 
 */
typedef struct {
  /// radio frequency (i.e US915Mhz)
  float band;
  /// slave select pin
  int ss;
  /// chip reset pin
  int rst;
  /// dio0 interrupt pin
  int di0;
  /// dio1 interrupt pin
  int di1;
  /// transmit power
  int txPower;
  /// interrupt service routine function when di0 activates
  void (*func)(void); 
} LoraConfigParams;

/**
 * @brief Internal LoRa chip abstraction.
 *
 * Provides internal access to the LoRa chip driver. This class is used by other
 * components of the CDP implementation.
 *
 */
class DuckLora {
public:
  /**
   * @brief Get a singletom instance of the DuckLora class,\.
   *
   * @returns A pointer to a DuckLora object
   */
  static DuckLora* getInstance();

  /**
   * @brief Initialize the LoRa chip.
   * 
   * @param config    lora configurstion parameters
   * @returns 0 if initialization was successful, an error code otherwise. 
   */
  int setupLoRa(LoraConfigParams config);
  
  /**
   * @brief Send packet data out into the LoRa mesh network
   *
   * @param data byte buffer to send
   * @param length length of the byte buffer
   * @return int
   */
  int sendData(byte* data, int length);

  /**
   * @brief Send packet data out into the LoRa mesh network
   *
   * @param data byte vector to send
   * @returns DUCK_ERR_NONE if the message was sent successfully, an error code otherwise.
   */
  int sendData(std::vector<byte> data);
  int sendData(DuckPacket* packet);
  
  /**
   * @brief Check if a received packet is available for processing.
   * 
   * @returns true if a packet was available, false otherwise.
   */
  bool loraPacketReceived();

  /**
   * @brief Set the Duck to be ready to recieve LoRa packets.
   *
   * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int startReceive();

  /**
   * @brief Set the Duck to be ready to transmit LoRa packets.
   *
   * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int transmitData();

  /**
   * @brief Set the Duck to be ready to transmit the given data
   *
   * @param data the data buffer to transmit
   * @param size the size of the data buffer
   * @return DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int transmitData(byte* data, int size);


  /**
   * @brief Get the current RSSI value.
   * 
   * @returns An integer representing the rssi value.
   */
  int getRSSI();

  
  /**
   * @brief Get the transmission buffer.
   * 
   * @returns An array of bytes containing the transmission packet
   */
  byte* getTransmissionBuffer() { return transmission; }

  /**
   * @brief Transmit a ping message.
   * 
   * @returns 0 if the message was sent sucessfully, an error code otherwise. 
   */
  int ping();

  /**
   * @brief Set the LoRa chip in standby mode.
   *
   * @returns DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an
   * error code otherwise.
   */
  int standBy();

  int getReceivedData(std::vector<byte>* packetBytes);

private:
  DuckLora();
  DuckLora(DuckLora const&) = delete;
  DuckLora& operator=(DuckLora const&) = delete;
  static DuckLora* instance;

  byte transmission[CDPCFG_CDP_BUFSIZE];
  CDP_Packet packet;
};

#endif
