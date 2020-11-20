#include "include/DuckPacket.h"
#include "DuckError.h"
#include "DuckLogger.h"
#include "include/DuckUtils.h"
#include "MemoryFree.h"
#include <string>

bool DuckPacket::prepareForRelaying(std::vector<byte> duid, std::vector<byte> dataBuffer) {

  bool relaying;
  int packet_length = dataBuffer.size();

  this->reset();

  loginfo("prepareForRelaying: START");
  logdbg("prepareForRelaying: building packet from received data...");

  // get data section and data crc
  byte* data = dataBuffer.data();
  int path_pos = data[PATH_OFFSET_POS];
  std::vector<byte> data_section;
  data_section.insert(data_section.end(), &data[DATA_POS], &data[path_pos]);
  uint32_t packet_data_crc = duckutils::toUnit32(&data[DATA_CRC_POS]);

  // build an rxPacket with data we have received
  // duid
  packet.duid.insert(packet.duid.end(), &dataBuffer[0], &dataBuffer[DUID_LENGTH]);
  // muid
  packet.muid.insert(packet.muid.end(), &dataBuffer[MUID_POS], &dataBuffer[TOPIC_POS]);
  // topic
  packet.topic = dataBuffer[TOPIC_POS];
  // path offset
  packet.path_offset = dataBuffer[PATH_OFFSET_POS];
  // reserved
  packet.reserved.insert(packet.reserved.end(), &dataBuffer[RESERVED_POS], &dataBuffer[DATA_POS]);
  // data crc
  packet.dcrc = packet_data_crc;
  // data section
  packet.data.insert(packet.data.end(), data_section.begin(), data_section.end());
  // path section
  packet.path.insert(packet.path.end(), &dataBuffer[path_pos], &dataBuffer[packet_length]);
  // update the rx packet byte buffer
  buffer.insert(buffer.end(), dataBuffer.begin(), dataBuffer.end());

  loginfo("prepareForRelaying: Packet is built. Checking for relay...");

  // when a packet is relayed the given duid is added to the path section of the packet
  relaying = relay(duid);
  if(!relaying) {
    this->reset();
  }
  loginfo("prepareForRelaying: DONE. Relay = "+ String (relaying));
  return relaying;
}

int DuckPacket::prepareForSending(byte topic, std::vector<byte> app_data) {

  uint8_t app_data_length = app_data.size();
  
  this->reset();

  if (topic < reservedTopic::max_reserved) {
    logerr("ERROR send data failed, topic is reserved.");
    return DUCKPACKET_ERR_TOPIC_INVALID;
  }
  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }

  loginfo("prepareForSending: DATA LENGTH: " + String(app_data_length) +
          " TOPIC: " + String(topic));

  byte message_id[MUID_LENGTH];
  duckutils::getRandomBytes(MUID_LENGTH, message_id);

  byte crc_bytes[DATA_CRC_LENGTH];
  //TODO: update the CRC32 library to return crc as a byte array
  uint32_t value = CRC32::calculate(app_data.data(), app_data.size());  
  crc_bytes[0] = (value >> 24) & 0xFF;
  crc_bytes[1] = (value >> 16) & 0xFF;
  crc_bytes[2] = (value >> 8) & 0xFF;
  crc_bytes[3] = value & 0xFF;
  
  // ----- insert packet header  -----
  // device uid
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  logdbg("Duid:      " + duckutils::convertToHex(duid.data(), duid.size()));

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

  // reserved
  buffer.insert(buffer.end(), 0x00);
  buffer.insert(buffer.end(), 0x00);
  logdbg("Reserved:  " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // data crc
  buffer.insert(buffer.end(), &crc_bytes[0], &crc_bytes[DATA_CRC_LENGTH]);
  logdbg("Data CRC:  " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // ----- insert data -----
  buffer.insert(buffer.end(), app_data.begin(), app_data.end());
  logdbg("Data:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  // ----- insert path -----
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  logdbg("Path:      " + duckutils::convertToHex(buffer.data(), buffer.size()));

  logdbg("Built packet: " + duckutils::convertToHex(buffer.data(), buffer.size()));
  return DUCK_ERR_NONE;
}

bool DuckPacket::relay(std::vector<byte> duid) {

  int path_length = packet.path.size();

  if (path_length == MAX_PATH_LENGTH) {
    logerr("ERROR Max hops reached. Cannot relay packet.");
    return false;
  }

  // we don't have a contains() method but we can use indexOf()
  // if the result is > 0 then the substring was found
  // starting at the returned index value.
  String id = duckutils::convertToHex(duid.data(), duid.size());
  String path = duckutils::convertToHex(packet.path.data(), packet.path.size());
  if (path.indexOf(id) >= 0) {
    loginfo("Packet already seen. ignore it.");
    return false;
  }

  // update the path so we are good to send the packet back into the mesh
  packet.path.insert(packet.path.end(), duid.begin(), duid.end());
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  return true;
}