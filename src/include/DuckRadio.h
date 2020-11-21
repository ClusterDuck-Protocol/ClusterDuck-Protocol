/**
 * @file DuckRadio.h
 * @brief This file is internal to CDP and provides the library access to
 * onboard LoRa module functions as well as packet management.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */

#ifndef DUCKLORA_H_
#define DUCKLORA_H_

#include "../DuckDisplay.h"
#include "../DuckError.h"
#include "../DuckLogger.h"
#include <Arduino.h>

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
  /// SPI slave select pin - the pin on each device that the master can use to enable and disable specific devices.
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
 * @brief Internal Radio chip abstraction.
 *
 * Provides internal access to the LoRa chip driver. This class is used by other
 * components of the CDP implementation.
 *
 */
class DuckRadio {

public:
  /**
   * @brief Get a singletom instance of the DuckRadio class,\.
   *
   * @returns A pointer to a DuckRadio object
   */
  static DuckRadio* getInstance();

  /**
   * @brief Initialize the LoRa chip.
   * 
   * @param config    lora configurstion parameters
   * @returns 0 if initialization was successful, an error code otherwise. 
   */
  int setupRadio(LoraConfigParams config);
  
  /**
   * @brief Send packet data out into the LoRa mesh network
   *
   * @param data byte buffer to send
   * @param length length of the byte buffer
   * @return int
   */
  int sendData(byte* data, int length);

  /**
   * @brief Send packet data out into the mesh network
   *
   * @param data byte vector to send
   * @returns DUCK_ERR_NONE if the message was sent successfully, an error code otherwise.
   */
  int sendData(std::vector<byte> data);
  
  /**
   * @brief Send packet data out into the mesh network
   *
   * @param packet Duckpacket object that contains the data to send
   * @return DUCK_ERR_NONE if the message was sent successfully, an error code otherwise.
   */
  int relayPacket(DuckPacket* packet);
  
  /**
   * @brief Set the Duck to be ready to recieve LoRa packets.
   *
   * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int startReceive();

  /**
   * @brief Set the Duck to be ready to transmit packets.
   *
   * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int startTransmitData();
  /**
   * @brief Set the Duck to be ready to transmit packets.
   *
   * @param data data to transmit
   * @param length data length in bytes
   * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int startTransmitData(byte* data, int length);

  /**
   * @brief Get the current RSSI value.
   *
   * @returns An integer representing the rssi value.
   */
  int getRSSI();

  /**
   * @brief Transmit a ping message.
   * 
   * @returns DUCK_ERR_NONE if the message was sent sucessfully, an error code otherwise. 
   */
  int ping();

  /**
   * @brief Set the LoRa chip in standby mode.
   *
   * @returns DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an
   * error code otherwise.
   */
  int standBy();

  /**
   * @brief Set the LoRa radio into sleep mode.
   * 
   * @returns DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an
   * error code otherwise.   
   */
  int sleep();
  
  /**
   * @brief Get the data received from the radio
   * 
   * @param  packetBytes byte buffer to contain the data 
   * @return DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an error code otherwise. 
   */
  int readReceivedData(std::vector<byte>* packetBytes);

  /**
   * @brief Process IRQ interrupts for the LoRa Radio.
   * 
   */
  void processRadioIrq();

private:
  DuckRadio();
  DuckRadio(DuckRadio const&) = delete;
  DuckRadio& operator=(DuckRadio const&) = delete;
  static DuckRadio* instance;
  DuckDisplay* display = DuckDisplay::getInstance();
};

#endif
