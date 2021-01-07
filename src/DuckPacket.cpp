#include "include/DuckPacket.h"
#include "DuckError.h"
#include "DuckLogger.h"
#include "MemoryFree.h"
#include "include/DuckUtils.h"
#include <string>


bool DuckPacket::prepareForRelaying(std::vector<byte> duid, std::vector<byte> dataBuffer) {

  bool relaying;


  int packet_length = dataBuffer.size();
  int path_pos = dataBuffer.data()[PATH_OFFSET_POS];

  std::vector<byte> path_section;

  // extract path section from the packet buffer
  path_section.assign(&dataBuffer[path_pos], &dataBuffer[packet_length]);

  this->reset();

  loginfo("prepareForRelaying: START");

  // update the rx packet byte buffer
  buffer.assign(dataBuffer.begin(), dataBuffer.end());

  loginfo("prepareForRelaying: Packet is built. Checking for relay...");

  // when a packet is relayed the given duid is added to the path section of the
  // packet
  relaying = relay(duid, path_section);
  if (!relaying) {
    this->reset();
  }
  loginfo("prepareForRelaying: DONE. Relay = " + String(relaying));
  return relaying;
}

bool DuckPacket::relay(std::vector<byte> duid, std::vector<byte> path_section) {

  int path_length = path_section.size();

  if (path_length == MAX_PATH_LENGTH) {
    logerr("ERROR Max hops reached. Cannot relay packet.");
    return false;
  }

  // we don't have a contains() method but we can use indexOf()
  // if the result is > 0 then the substring was found
  // starting at the returned index value.
  String id = duckutils::convertToHex(duid.data(), duid.size());
  String path_string = duckutils::convertToHex(path_section.data(), path_length);
  if (path_string.indexOf(id) >= 0) {
    loginfo("Packet already seen. ignore it.");
    return false;
  }
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  buffer[HOP_COUNT_POS]++;
  return true;
}

int DuckPacket::prepareForSending(std::vector<byte> targetDevice, byte duckType, byte topic, std::vector<byte> app_data) {

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
  // TODO: update the CRC32 library to return crc as a byte array
  uint32_t value = CRC32::calculate(app_data.data(), app_data.size());
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
  buffer.insert(buffer.end(), app_data.begin(), app_data.end());
  logdbg("Data:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // ----- insert path -----
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  logdbg("Path:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  logdbg("Built packet: " +
         duckutils::convertToHex(buffer.data(), buffer.size()));
  return DUCK_ERR_NONE;
}

