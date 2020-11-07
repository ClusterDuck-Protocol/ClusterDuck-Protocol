#include "include/DuckPacket.h"
#include "DuckError.h"
#include "DuckLogger.h"
#include "include/DuckUtils.h"
#include <string>

bool DuckPacket::update(std::vector<byte> duid, std::vector<byte> dataBuffer) {

  bool relaying;
  byte path_pos;
  byte packet_length = dataBuffer.size();
  std::vector<byte> data_section;

  logdbg("RX buffer: " +
         String(duckutils::convertToHex(dataBuffer.data(), dataBuffer.size())));

  if (packet_length < MIN_PACKET_LENGTH) {
    logerr("ERROR Packet size is invalid: (" + String(packet_length) +
           ") Data may be corrupted.");
    return false;
  }

  path_pos = dataBuffer[PATH_OFFSET_POS];
  data_section.insert(data_section.end(), &dataBuffer[DATA_POS], &dataBuffer[path_pos]);
  packet.dcrc = duckutils::toUnit32(&dataBuffer[DATA_CRC_POS]);
  logdbg("DATA SECTION: " + String(duckutils::convertToHex(
                                data_section.data(), data_section.size())));
  uint32_t computed_crc = CRC32::calculate(data_section.data(), data_section.size());

  if (computed_crc != packet.dcrc) {
    logerr("ERROR data crc mismatch: was: "+ String(packet.dcrc) + " expected:" + String(computed_crc));
    return false;
  }

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

  // data section
  packet.data.insert(packet.data.end(), data_section.begin(), data_section.end());
  // path section
  packet.path.insert(packet.path.end(), &dataBuffer[path_pos], &dataBuffer[packet_length]);
  // update the rx packet byte buffer
  buffer.insert(buffer.end(), dataBuffer.begin(), dataBuffer.end());

  //logdbg("Current path: " + String(duckutils::convertToHex(packet.path.data(), packet.path.size())));

  // check if we need to relay the packet
  relaying = relay(duid);
  //logdbg("Updated path: " + String(duckutils::convertToHex(packet.path.data(), packet.path.size())));
  logdbg("Updated buffer: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  return relaying;
}

int DuckPacket::buildPacketBuffer(byte topic, std::vector<byte> app_data) {


  uint8_t app_data_length = app_data.size();

  loginfo("buildPacketBuffer: DATA LENGTH: " + String(app_data_length) +
          " TOPIC: " + String(topic));

  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }
 
  byte message_id[MUID_LENGTH];
  duckutils::getRandomBytes(MUID_LENGTH, message_id);

  byte crc_bytes[DATA_CRC_LENGTH];
  uint32_t value = CRC32::calculate(app_data.data(), app_data.size());
  crc_bytes[0] = (value >> 24) & 0xFF;
  crc_bytes[1] = (value >> 16) & 0xFF;
  crc_bytes[2] = (value >> 8) & 0xFF;
  crc_bytes[3] = value & 0xFF;
  
  // ----- insert packet header  -----
  // device uid
  buffer.insert(buffer.end(), duid.begin(), duid.end());
   logdbg("Duid:      " + String(duckutils::convertToHex(duid.data(), duid.size())));

  // message uid
  buffer.insert(buffer.end(), &message_id[0], &message_id[MUID_LENGTH]);
  logdbg("Muid:      " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // topic
  buffer.insert(buffer.end(), topic);
  logdbg("Topic:     " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // path offset
  byte offset = HEADER_LENGTH + app_data_length;
  buffer.insert(buffer.end(), offset);
  logdbg("Offset:    " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // reserved
  buffer.insert(buffer.end(), 0x00);
  buffer.insert(buffer.end(), 0x00);
  logdbg("Reserved:  " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // data crc
  buffer.insert(buffer.end(), &crc_bytes[0], &crc_bytes[DATA_CRC_LENGTH]);
  logdbg("Data CRC:  " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // ----- insert data -----
  buffer.insert(buffer.end(), app_data.begin(), app_data.end());
  logdbg("Data:      " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // ----- insert path -----
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  logdbg("Path:      " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  logdbg("Built packet: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  return DUCK_ERR_NONE;
}

// checks the current packet against a duid to determine if the
// packet needs to be send back into the mesh for the next hop.
bool DuckPacket::relay(std::vector<byte> duid) {

  int hops = packet.path.size() / DUID_LENGTH;
  if (hops >= MAX_HOPS) {
    logerr("ERROR Max hops reached. Cannot relay packet.");
    return false;
  }

  if ((packet.path.size() + DUID_LENGTH) > MAX_PATH_LENGTH) {
    logerr("ERROR Path section corrupted. Cannot relay packet.");
    return false;
  }
  // we don't have a contains() method but we can use indexOf()
  // if the result is 0 or greater then the substring was found
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