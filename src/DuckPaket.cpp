#include "include/DuckPacket.h"
#include "include/DuckUtils.h"
#include "DuckError.h"

bool DuckPacket::update(std::vector<byte> duid, std::vector<byte> dataBuffer) {
  
  bool relaying;

  byte packet_length = dataBuffer.size();
  byte path_offset = dataBuffer[PATH_OFFSET_POS];

  packet.duid.insert(packet.duid.end(), &dataBuffer[0], &dataBuffer[DUID_LENGTH]);
  packet.muid.insert(packet.muid.end(), &dataBuffer[MUID_POS], &dataBuffer[TOPIC_POS]);
  packet.topic = dataBuffer[TOPIC_POS];
  packet.path_offset = dataBuffer[PATH_OFFSET_POS];
  packet.reserved.insert(packet.reserved.end(), &dataBuffer[RESERVED_POS], &dataBuffer[DATA_POS]);
  packet.data.insert(packet.data.end(), &dataBuffer[DATA_POS], &dataBuffer[path_offset]);
  packet.path.insert(packet.path.end(), &dataBuffer[path_offset], &dataBuffer[packet_length]);
  buffer.insert(buffer.end(), dataBuffer.begin(), dataBuffer.end());

  relaying = relay(duid);
  return relaying;
}

int DuckPacket::buildPacketBuffer(byte topic, std::vector<byte> app_data) {

  uint8_t app_data_length = app_data.size();

  Serial.print("[DuckPacket] buildPacket() app_data len: ");
  Serial.print(app_data_length);
  Serial.print(" topic: ");
  Serial.println(topic);
  
  if (topic < resevedTopic::max_reserved) {
    return DUCKPACKET_ERR_TOPIC_INVALID;
  }

  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }
  byte message_id[MUID_LENGTH];

  duckutils::getRandomBytes(MUID_LENGTH, message_id);

  // ----- insert packet header  -----
  // device uid
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  // Serial.println("[DuckPacket] duid: " + String(duckutils::convertToHex(duid.data(), duid.size())));

  // message uid
  buffer.insert(buffer.end(), &message_id[0], &message_id[MUID_LENGTH]);
  // Serial.println("[DuckPacket] muid: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  // topic
  buffer.insert(buffer.end(), topic);
  // Serial.println("[DuckPacket] topic: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  // path offset
  byte offset = HEADER_LENGTH + app_data_length;
  buffer.insert(buffer.end(), offset);
  // Serial.println("[DuckPacket] offset: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  // reserved
  buffer.insert(buffer.end(), 0x00);
  buffer.insert(buffer.end(), 0x00);
  // Serial.println("[DuckPacket] resevered: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // ----- insert data -----
  buffer.insert(buffer.end(), app_data.begin(), app_data.end());
  // Serial.println("[DuckPacket] data: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  // ----- insert path -----
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  //Serial.println("[DuckPacket] path: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));

  Serial.println("[DuckPacket] packet: " + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  return DUCK_ERR_NONE;
}

// checks the current packet against a duid to determine if the
// packet needs to be send back into the mesh for the next hop.
bool  DuckPacket::relay(std::vector<byte> duid) {
  
  int hops = packet.path.size() / DUID_LENGTH;
  if (hops >= MAX_HOPS) {
    Serial.println("[DuckPacket] Oops. Max hops reached. Cannot relay packet.");
    return false;
  }

  String id = duckutils::convertToHex(duid.data(), duid.size());
  String paths = duckutils::convertToHex(packet.path.data(), packet.path.size());

  // we don't have a contains() method. but we can use indexOf
  // if the result is 0 or greater then the substring was found
  // starting at the returned index value.
  if (paths.indexOf(id) >= 0) {
    return false;
  }
  // update the path so we are good to send the packet back into the mesh
  packet.path.insert(packet.path.end(), duid.begin(), duid.end());
  buffer.insert(buffer.end(), duid.begin(), duid.end());
  return true;
}