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
 
public:
  
  static DuckRadio& getInstance() {
    static DuckRadio instance; 
    return instance;
  }
 
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
   * @returns DUCK_ERR_NONE if the sync word was set successfully, an error code otherwise.
   */
  int setSyncWord(byte syncWord);

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
   * @returns DUCK_ERR_NONE if the channel was set successfully, an error code otherwise.
   */
  int setChannel(int channelNum);

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
   * @brief Get the DuckRadio channel.
   * 
   * @return int channel number.
   */
  int getChannel() { return channel; }

  /**
   * @brief Service the RadioLib SX127x and SX126x interrupt flags.
   * 
   */
  void serviceInterruptFlags();

  /*
   * @brief Interrupt service routine for the LoRa module.
   *
  */
  static void onInterrupt();

  /**
   * @brief Get the data receive flag.
   * 
   * @return true if the flag is set, false otherwise.
   */
  static bool getReceiveFlag() { return receivedFlag; }
 

private:

  static DuckRadio* instance;
  int channel;  
  static volatile uint16_t interruptFlags;
  static volatile bool receivedFlag;
  volatile bool isSetup = false;

  static void setReceiveFlag(bool value) { receivedFlag = value; }

  
  int goToReceiveMode(bool clear);
  int checkLoRaParameters(LoraConfigParams config);

  DuckRadio() {};
  DuckRadio(DuckRadio const&) = delete;
  DuckRadio& operator=(DuckRadio const&) = delete;
  
};

#endif
