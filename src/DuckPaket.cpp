#include "include/DuckPacket.h"
#include "include/DuckUtils.h"
#include "DuckError.h"

int DuckPacket::buildDataBuffer(byte topic, byte* app_data) {
  uint8_t app_data_length = sizeof(app_data) / sizeof(byte);

  if (topic < resevedTopic::max_topic) {
    return DUCKPACKET_ERR_TOPIC_INVALID;
    }

  if (app_data_length > MAX_DATA_LENGTH) {
    return DUCKPACKET_ERR_SIZE_INVALID;
  }

  String muid = duckutils::createUuid(MUID_LENGTH);
  // insert packet header
  // device uid
  buffer.insert(buffer.end(), deviceId.begin(), deviceId.end());
  // message uid
  buffer.insert(buffer.end(), muid.begin(), muid.end());
  // topic
  buffer.insert(buffer.end(), topic);
  // path offset
  buffer.insert(buffer.end(), (byte)(HEADER_LENGTH + app_data_length));
  // reserved
  buffer.insert(buffer.end(), 0x00);
  buffer.insert(buffer.end(), 0x00);

  // insert data
  buffer.insert(buffer.end(), &app_data[0], &app_data[app_data_length]);
  // insert path
  buffer.insert(buffer.end(), deviceId.begin(), deviceId.end());

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