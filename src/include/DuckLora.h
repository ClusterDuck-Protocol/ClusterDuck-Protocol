#ifndef DUCKLORA_H_
#define DUCKLORA_H_

#include <Arduino.h>
#include <RadioLib.h>

#include "DuckDisplay.h"
#include "LoraPacket.h"
#include "cdpcfg.h"

#define DUCKLORA_ERR_NONE 0
#define DUCKLORA_ERR_BEGIN -1
#define DUCKLORA_ERR_SETUP -2
#define DUCKLORA_ERR_RECEIVE -3
#define DUCKLORA_ERR_TIMEOUT -4
#define DUCKLORA_ERR_TRANSMIT -5

#define DUCKLORA_ERR_HANDLE_PACKET -100
#define DUCKLORA_ERR_MSG_TOO_LARGE -101

const byte ping_B = 0xF4;
const byte senderId_B = 0xF5;
const byte topic_B = 0xE3;
const byte messageId_B = 0xF6;
const byte payload_B = 0xF7;
const byte iamhere_B = 0xF8;
const byte path_B = 0xF3;

typedef struct {
  float band; // radio frequency (i.e US915Mhz)
  int ss;     // slave select pin
  int rst;    // chip reset pin
  int di0;    // dio0 interrupt pin 
  int di1;    // dio1 interrupt pin
  int txPower;// transmit power
  void (*func)(void); // interrupt service routine function when di0 activates
} LoraConfigParams;

/**
 * LoRa chip abstraction.
 *
 * Provides internal access to the LoRa chip driver. This class is used by other components
 * of the CDP implementation.
 *
 */
class DuckLora {
public:
  /**
   * @brief Get a singletom instance of the DuckLora class,\.
   *
   * @return A pointer to a DuckLora object
   */
  static DuckLora* getInstance();

  /**
   * @brief Initialize the LoRa chip.
   * 
   * @param config    lora configurstion parameters
   * @param deviceId  device id
   * @return 0 if initialization was successful, an error code otherwise. 
   */
  int setupLoRa(LoraConfigParams config, String deviceId);
  
  /**
   * @brief Handle a Duck LoRa packet.
   * 
   * @return 0 if handling was successful, an error code otherwise. 
   */
  int handlePacket();

  /**
   * @brief Get the last received LoRa packet.
   * 
   * @param pSize size of the packet
   * @return A string representing the last received message .
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
   * @return 0 if the message was sent successfully, an error code otherwise. 
   */
  int sendPayloadStandard(String msg = "", String topic = "",
                          String senderId = "", String messageId = "",
                          String path = "");

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
   * @brief Check if a received packet is available for processing.
   * 
   * @return true if a packet was available, false otherwise.
   */
  bool loraPacketReceived();
  
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
   * @brief Reset the index of the received Packet buffer.
   * 
   */
  void resetPacketIndex() { _packetIndex = 0; }

  /**
   * @brief Get the current packet buffer index.
   * 
   * @return Current index in the transmission buffer.
   */
  int getPacketIndex() { return _packetIndex; }
  
  /**
   * @brief Get the transmission buffer.
   * 
   * @return An array of bytes containing the transmission packet
   */
  byte* getTransmissionBuffer() { return _transmission; }

  /**
   * @brief Get the transmited byte at the given index in the transmission buffer.
   * 
   * @param index position in the transmission buffer
   * @return The byte value in the transmission buffer at the given index.
   */
  byte getTransmitedByte(int index) { return _transmission[index]; }

  /**
   * @brief Transmit a ping message.
   * 
   * @return 0 if the message was sent sucessfully, an error code otherwise. 
   */
  int ping();

  /**
   * @brief Set the LoRa chip in standby mode.
   *
   * @return 0 if the chip is sucessfuly set in standby mode, an error code otherwise.
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
