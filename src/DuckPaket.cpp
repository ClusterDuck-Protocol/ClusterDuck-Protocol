#include "include/DuckPacket.h"
#include "include/DuckUtils.h"
#include "DuckError.h"

enum resevedTopic {
  unused        = 0x00,
  ping          = 0x01,
  pong          = 0x02,
  max_reserved  = 0x0F
};


int DuckPacket::buildDataBuffer(byte topic, std::vector<byte> app_data) {

  int app_data_length = app_data.size();

  Serial.print("buildDataBuffer() msg len: ");
  Serial.println(app_data_length);

  if (topic < resevedTopic::max_reserved) {
    return DUCKPACKET_ERR_TOPIC_INVALID;
  }

  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }
  byte message_id[MUID_LENGTH];
  duckutils::getRandomBytes(MUID_LENGTH, message_id);
  // insert packet header
  // device uid
  buffer.insert(buffer.end(), deviceId.begin(), deviceId.end());

  // message uid
  buffer.insert(buffer.end(), &message_id[0], &message_id[MUID_LENGTH - 1]);
  // topic
  buffer.insert(buffer.end(), topic);
  // path offset
  buffer.insert(buffer.end(), (byte)(HEADER_LENGTH + app_data_length));
  // reserved
  buffer.insert(buffer.end(), 0x00);
  buffer.insert(buffer.end(), 0x00);
  // insert data
  buffer.insert(buffer.end(), app_data.begin(), app_data.end());
  // insert path
  buffer.insert(buffer.end(), deviceId.begin(), deviceId.end());

  Serial.println("msg:" + String(duckutils::convertToHex(buffer.data(), buffer.size())));
  
  return DUCK_ERR_NONE;
}

int DuckPacket::updatePath() {
  byte path_offset = buffer[PATH_OFFSET_POS];
  int hops = (buffer.size() - path_offset) / DUID_LENGTH;

  if (hops >= MAX_HOPS) {
    return DUCKPACKET_ERR_MAX_HOPS;
  }
  buffer.insert(buffer.end(), deviceId.begin(), deviceId.end());
  return DUCK_ERR_NONE;
}