#include "Arduino.h"
#include "include/DuckPacket.h"
#include "DuckError.h"
#include "DuckLogger.h"
#include "MemoryFree.h"
#include "include/DuckUtils.h"
#include "include/DuckCrypto.h"
#include "include/bloomfilter.h"
#include <string>


bool DuckPacket::prepareForRelaying(BloomFilter *filter, std::vector<byte> dataBuffer) {


  this->reset();

  loginfo_ln("prepareForRelaying: START");
  loginfo_ln("prepareForRelaying: Packet is built. Checking for relay...");

  // Query the existence of strings
  bool alreadySeen = filter->bloom_check(&dataBuffer[MUID_POS], MUID_LENGTH);
  if (alreadySeen) {
    logdbg_ln("handleReceivedPacket: Packet already seen. No relay.");
    return false;
  } else {
    filter->bloom_add(&dataBuffer[MUID_POS], MUID_LENGTH);
    logdbg_ln("handleReceivedPacket: Relaying packet: %s", duckutils::convertToHex(&dataBuffer[MUID_POS], MUID_LENGTH).c_str());
  }

  // update the rx packet internal byte buffer
  buffer = dataBuffer;
  int hops = buffer[HOP_COUNT_POS]++;
  loginfo_ln("prepareForRelaying: hops count: %d", hops);
  return true;
  
  
}

void DuckPacket::getUniqueMessageId(BloomFilter * filter, byte message_id[MUID_LENGTH]) {

  bool getNewUnique = true;
  while (getNewUnique) {
    duckutils::getRandomBytes(MUID_LENGTH, message_id);
    getNewUnique = filter->bloom_check(message_id, MUID_LENGTH);
    loginfo_ln("prepareForSending: new MUID -> %s",duckutils::convertToHex(message_id, MUID_LENGTH).c_str());
  }
}

int DuckPacket::prepareForSending(BloomFilter *filter,
                                  std::array<byte,8> targetDevice, byte duckType,
                                  byte topic, std::vector<byte> app_data) {

  std::vector<uint8_t> encryptedData;
  uint8_t app_data_length = app_data.size();

  this->reset();

  if ( app_data.empty() || app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }

  loginfo_ln("prepareForSending: DATA LENGTH: %d - TOPIC (%s)", app_data_length, CdpPacket::topicToString(topic).c_str());

  byte message_id[MUID_LENGTH];
  getUniqueMessageId(filter, message_id);

  byte crc_bytes[DATA_CRC_LENGTH];
  uint32_t value;
  // TODO: update the CRC32 library to return crc as a byte array
  if(duckcrypto::getState()) {
    duckcrypto::encryptData(app_data.data(), encryptedData.data(), app_data.size());
    value = CRC32::calculate(encryptedData.data(), encryptedData.size());
  } else {
    value = CRC32::calculate(app_data.data(), app_data.size());
  }
  
  crc_bytes[0] = (value >> 24) & 0xFF;
  crc_bytes[1] = (value >> 16) & 0xFF;
  crc_bytes[2] = (value >> 8) & 0xFF;
  crc_bytes[3] = value & 0xFF;

  // ----- insert packet header  -----
  // source device uid

  for(int i = 0; i < DUID_LENGTH; i++) {
    buffer[SDUID_POS + i] = duid[i];
  }
  logdbg_ln("SDuid:     %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());
  // destination device uid
  for(int i = 0; i < DUID_LENGTH; i++) {
    buffer[DDUID_POS + i] = targetDevice[i];
  }
  logdbg_ln("DDuid:     %s", duckutils::convertToHex(targetDevice.data(), targetDevice.size()).c_str());

  // message uid
  for(int i = 0; i < MUID_LENGTH; i++) {
    buffer[MUID_POS + i] = message_id[i];
  }
  logdbg_ln("Muid:      %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

  // topic
  buffer[TOPIC_POS] = topic;
  logdbg_ln("Topic:     %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

  // duckType
    buffer[DUCK_TYPE_POS] = duckType;
  logdbg_ln("duck type: %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

  // hop count
  buffer[HOP_COUNT_POS] = 0x00;
  logdbg_ln("hop count: %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

  // data crc
  for(int i = 0; i < DATA_CRC_LENGTH; i++) {
    buffer[DATA_CRC_POS + i] = crc_bytes[i];
  }
  logdbg_ln("Data CRC:  %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

  // ----- insert data -----
  if(duckcrypto::getState()) {

   for(int i = 0; i < MAX_DATA_LENGTH; i++) {
       buffer[DATA_POS + i] = encryptedData[i];
   }
    logdbg_ln("Encrypted Data:      %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

  } else {
    for(int i = 0; i < app_data.size(); i++) {
      buffer[DATA_POS + i] = app_data[i];
    }
    logdbg_ln("Data:      %s",duckutils::convertToHex(buffer.data(), buffer.size()).c_str());
  }
  
  logdbg_ln("Built packet: %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());
  return DUCK_ERR_NONE;
}

