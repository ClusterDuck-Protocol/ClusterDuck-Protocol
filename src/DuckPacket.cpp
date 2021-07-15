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

  loginfo("prepareForRelaying: START");
  loginfo("prepareForRelaying: Packet is built. Checking for relay...");

  //TODO: Add bloom filter empty when full
  //TODO: Add 2nd bloom filter
  //TODO: Calculate false positive chance
  //TODO: Add backwards compatibility

  // Query the existence of strings
  // bool alreadySeen = filter->contains(&dataBuffer[MUID_POS], MUID_LENGTH);
  bool alreadySeen = bloom_check(filter, &dataBuffer[MUID_POS], MUID_LENGTH);
  if (!alreadySeen) {
    logdbg("handleReceivedPacket: Packet already seen. No relay.");
    return false;
  } else {
    bloom_add(filter, &dataBuffer[MUID_POS], MUID_LENGTH);
    logdbg("handleReceivedPacket: Relaying packet: "  + duckutils::convertToHex(&dataBuffer[MUID_POS], MUID_LENGTH));
  }

  // update the rx packet internal byte buffer
  buffer.assign(dataBuffer.begin(), dataBuffer.end());
  int hops = buffer[HOP_COUNT_POS]++;
  loginfo("prepareForRelaying: hops count: "+ String(hops));
  return true;
  
  
}

int DuckPacket::prepareForSending(std::vector<byte> targetDevice, byte duckType, byte topic, std::vector<byte> app_data) {

  std::vector<uint8_t> encryptedData;
  uint8_t app_data_length = app_data.size();

  this->reset();

  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }

  loginfo("prepareForSending: DATA LENGTH: " + String(app_data_length) +
          " TOPIC: " + String(topic));

  byte message_id[MUID_LENGTH];
  duckutils::getRandomBytes(MUID_LENGTH, message_id);

  byte crc_bytes[DATA_CRC_LENGTH];
  uint32_t value;
  // TODO: update the CRC32 library to return crc as a byte array
  if(duckcrypto::getState()) {
    encryptedData.resize(app_data.size());
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
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  logdbg("SDuid:     " + duckutils::convertToHex(duid.data(), duid.size()));

  // destination device uid
  buffer.insert(buffer.end(), targetDevice.begin(), targetDevice.end());
  logdbg("DDuid:     " + duckutils::convertToHex(targetDevice.data(), targetDevice.size()));

  // message uid
  buffer.insert(buffer.end(), &message_id[0], &message_id[MUID_LENGTH]);
  logdbg("Muid:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // topic
  buffer.insert(buffer.end(), topic);
  logdbg("Topic:     " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // path offset
  byte offset = HEADER_LENGTH + app_data_length;
  buffer.insert(buffer.end(), offset);
  logdbg("Offset:    " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // duckType
  buffer.insert(buffer.end(), duckType);
  logdbg("duck type: " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // hop count
  buffer.insert(buffer.end(), 0x00);
  logdbg("hop count: " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // data crc
  buffer.insert(buffer.end(), &crc_bytes[0], &crc_bytes[DATA_CRC_LENGTH]);
  logdbg("Data CRC:  " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // ----- insert data -----
  if(duckcrypto::getState()) {

    buffer.insert(buffer.end(), encryptedData.begin(), encryptedData.end());
    logdbg("Encrypted Data:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  } else {
    buffer.insert(buffer.end(), app_data.begin(), app_data.end());
    logdbg("Data:      " + duckutils::convertToHex(buffer.data(), buffer.size()));
  }
  
  // ----- insert path -----
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  logdbg("Path:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  logdbg("Built packet: " +
         duckutils::convertToHex(buffer.data(), buffer.size()));
  return DUCK_ERR_NONE;
}

