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

#include "LoraPacket.h"
#include "cdpcfg.h"

const byte ping_B = 0xF4;
const byte senderId_B = 0xF5;
const byte topic_B = 0xE3;
const byte messageId_B = 0xF6;
const byte payload_B = 0xF7;
const byte iamhere_B = 0xF8;
const byte path_B = 0xF3;

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
   * @param deviceId  device id
   * @returns 0 if initialization was successful, an error code otherwise. 
   */
  int setupLoRa(LoraConfigParams config, String deviceId);
  
  /**
   * @brief Handle a Duck LoRa packet.
   * 
   * @returns 0 if handling was successful, an error code otherwise. 
   */
  int handlePacket();

  /**
   * @brief Get the last received LoRa packet.
   * 
   * @param pSize size of the packet
   * @returns A string representing the last received message.
   */
  String getPacketData(int pSize);

  /**
   * @brief  Send a message out into the LoRa mesh network.
   *
   * @param msg         payload representing the message
   * @param topic       the message topic
   * @param senderId    the device_id of the sender
   * @param messageId   the id of the message
   * @param path        a comma separated list of devide ids having seens the message
   * @returns DUCK_ERR_NONE if the message was sent successfully, an error code
   * otherwise.
   */
  int sendPayloadStandard(String msg = "", String topic = "",
                          String senderId = "", String messageId = "",
                          String path = "");

  /**
   * @brief Get the last received LoRa packet.
   * 
   * @returns A Packet object containing the last received message.
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
   * @returns true if the id is in the path, false otherwise.
   */
  bool idInPath(String path);

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
   * @brief Get the current RSSI value.
   * 
   * @returns An integer representing the rssi value.
   */
  int getRSSI();

  /**
   * @brief Reset the index of the received Packet buffer.
   * 
   */
  void resetPacketIndex() { _packetIndex = 0; }

  /**
   * @brief Get the current packet buffer index.
   * 
   * @returns Current index in the transmission buffer.
   */
  int getPacketIndex() { return _packetIndex; }
  
  /**
   * @brief Get the transmission buffer.
   * 
   * @returns An array of bytes containing the transmission packet
   */
  byte* getTransmissionBuffer() { return _transmission; }

  /**
   * @brief Get the transmited byte at the given index in the transmission buffer.
   * 
   * @param index position in the transmission buffer
   * @returns The byte value in the transmission buffer at the given index.
   */
  byte getTransmitedByte(int index) { return _transmission[index]; }

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

  /**
   * @brief Clear and Reset the transmission buffer.
   * 
   */
  void resetTransmissionBuffer();

private:
  byte _transmission[CDPCFG_CDP_BUFSIZE];
  int _packetIndex = 0;
  Packet _lastPacket;
  String _deviceId = "";
  int _availableBytes = 0;
  int _packetSize = 0;
  void resetLastPacket();

  DuckLora();
  DuckLora(DuckLora const&) = delete;
  DuckLora& operator=(DuckLora const&) = delete;
  static DuckLora* instance;
};

#endif
