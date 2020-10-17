#include "include/DuckPacket.h"
#include "include/DuckUtils.h"
#include "DuckError.h"
#include "DuckLogger.h"

bool DuckPacket::update(std::vector<byte> duid, std::vector<byte> dataBuffer) {
  
  bool relaying;

  byte packet_length = dataBuffer.size();
  byte path_offset = dataBuffer[PATH_OFFSET_POS];
  // build an rxPacket with data we have received
  packet.duid.insert(packet.duid.end(), &dataBuffer[0], &dataBuffer[DUID_LENGTH]);
  packet.muid.insert(packet.muid.end(), &dataBuffer[MUID_POS], &dataBuffer[TOPIC_POS]);
  packet.topic = dataBuffer[TOPIC_POS];
  packet.path_offset = dataBuffer[PATH_OFFSET_POS];
  packet.reserved.insert(packet.reserved.end(), &dataBuffer[RESERVED_POS], &dataBuffer[DATA_POS]);
  packet.data.insert(packet.data.end(), &dataBuffer[DATA_POS], &dataBuffer[path_offset]);
  packet.path.insert(packet.path.end(), &dataBuffer[path_offset], &dataBuffer[packet_length]);
  // update the rx packet byte buffer
  buffer.insert(buffer.end(), dataBuffer.begin(), dataBuffer.end());
  //logdbg_ln("Current path: " + String(duckutils::convertToHex(packet.path.data(), packet.path.size())));

  // check if we need to relay the packet
  relaying = relay(duid);
  //logdbg_ln("Updated path: " + String(duckutils::convertToHex(packet.path.data(), packet.path.size())));
  //logdbg_ln("Updated buffer: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  return relaying;
}

int DuckPacket::buildPacketBuffer(byte topic, std::vector<byte> app_data) {

  uint8_t app_data_length = app_data.size();

  logdbg_ln("buildPacket() app_data len: " + String(app_data_length) +
         " topic: " + String(topic) + "length: " + String(app_data_length));


  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }
  byte message_id[MUID_LENGTH];

  duckutils::getRandomBytes(MUID_LENGTH, message_id);

  // ----- insert packet header  -----
  // device uid
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  //logdbg_ln("Duid: " + String(duckutils::convertToHex(duid.data(), duid.size())));

  // message uid
  buffer.insert(buffer.end(), &message_id[0], &message_id[MUID_LENGTH]);
  //logdbg_ln("Muid: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  
  // topic
  buffer.insert(buffer.end(), topic);
  //logdbg_ln("Topic: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  
  // path offset
  byte offset = HEADER_LENGTH + app_data_length;
  buffer.insert(buffer.end(), offset);
  //logdbg_ln("Offset: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  
  // reserved
  buffer.insert(buffer.end(), 0x00);
  buffer.insert(buffer.end(), 0x00);
  //logdbg_ln("Reserved: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // ----- insert data -----
  buffer.insert(buffer.end(), app_data.begin(), app_data.end());
  //logdbg_ln("Data: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // ----- insert path -----
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  //logdbg_ln("Path: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  loginfo_ln("Built packet: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  return DUCK_ERR_NONE;
}

// checks the current packet against a duid to determine if the
// packet needs to be send back into the mesh for the next hop.
bool  DuckPacket::relay(std::vector<byte> duid) {
  
  int hops = packet.path.size() / DUID_LENGTH;
  if (hops >= MAX_HOPS) {
    logerr_ln("Max hops reached. Cannot relay packet.");
    return false;
  }

  // we don't have a contains() method but we can use indexOf()
  // if the result is 0 or greater then the substring was found
  // starting at the returned index value.
  String id = duckutils::convertToHex(duid.data(), duid.size());
  String path = duckutils::convertToHex(packet.path.data(), packet.path.size());
  if (path.indexOf(id) >= 0) {
    logdbg_ln("Packet already seen. ignore it.");
    return false;
  }

  // update the path so we are good to send the packet back into the mesh
  packet.path.insert(packet.path.end(), duid.begin(), duid.end());
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  return true;
}