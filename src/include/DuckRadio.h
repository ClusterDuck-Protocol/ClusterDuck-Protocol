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

#include <Arduino.h>

#include "../DuckDisplay.h"
#include "../DuckError.h"
#include "../DuckLogger.h"
#include "cdpcfg.h"
#include "DuckPacket.h"
#include "LoraPacket.h"

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
  int8_t txPower;
  /// bandwidth
  float bw;
  /// spreading factor
  uint8_t sf;
  /// gain
  uint8_t gain;
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
  friend class Duck;
  friend class DuckDetect;
  friend class DuckLink;
  friend class MamaDuck;
  friend class PapaDuck;

private:
  // Everything is private to force Duck (and Duck descendants) to be the only
  // way to interact with the radio. There should only be one Duck per sketch
  // so that arbitrary pieces of code cannot interfere with the radio. Also,
  // Duck does things like recording the outgoing MUIDs so that it can wait for
  // acknowledgments to those MUIDs.

  DuckRadio();

  /**
   * @brief Initialize the LoRa chip.
   * 
   * @param config    lora configurstion parameters
   * @returns 0 if initialization was successful, an error code otherwise. 
   */
  int setupRadio(LoraConfigParams config);

  /**
   * @brief Set sync word used to communicate between radios. 0x12 for private and 0x34 for public channels.
   * 
   * @param syncWord set byte syncWord
   */
  void setSyncWord(byte syncWord);

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
   * @param data data to transmit
   * @param length data length in bytes
   * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
   */
  int startTransmitData(byte* data, int length);

   /**
   * @brief change the duck channel.
   * 
   * @param channelNum set the channel number 1-6.
   */
  void setChannel(int channelNum);

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

  int getChannel() { return channel; }

private:
  static volatile uint16_t interruptFlags;
  void serviceInterruptFlags();
  static void onInterrupt();

  static volatile bool receivedFlag;
  static void setReceiveFlag(bool value) { receivedFlag = value; }
  static bool getReceiveFlag() { return receivedFlag; }

  DuckRadio(DuckRadio const&) = delete;
  DuckRadio& operator=(DuckRadio const&) = delete;

  DuckDisplay* display = DuckDisplay::getInstance();
  int err;
  int channel;  
};

#endif
