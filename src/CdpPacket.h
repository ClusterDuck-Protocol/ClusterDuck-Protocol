#ifndef CDPPACKET_H
#define CDPPACKET_H

#include "Arduino.h"
#include "include/DuckUtils.h"
#include "DuckLogger.h"
#include <string>
#include <vector>

#define MAX_HOPS 6

// field/section length (in bytes)
#define PACKET_LENGTH 256
#define DUID_LENGTH 8
#define MUID_LENGTH 4
#define DATA_CRC_LENGTH 4
#define HEADER_LENGTH 20

// field/section offsets
#define DUID_POS 0
#define MUID_POS 8
#define TOPIC_POS 12
#define PATH_OFFSET_POS 13
#define RESERVED_POS 14
#define DATA_CRC_POS 16
#define DATA_POS HEADER_LENGTH // Data section starts immediately after header

#define RESERVED_LENGTH 2
#define MAX_PATH_LENGTH (MAX_HOPS * DUID_LENGTH)
#define MAX_DATA_LENGTH (PACKET_LENGTH - HEADER_LENGTH - MAX_PATH_LENGTH)
#define MAX_PATH_OFFSET (PACKET_LENGTH - DUID_LENGTH - 1)

// header + 1 hop + 1 byte data
#define MIN_PACKET_LENGTH (HEADER_LENGTH + DUID_LENGTH + 1)

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

/**
 * @brief Defines preset topics for duck packets.
 *
 */
enum topics {
  /// generic message (e.g non emergency messages)
  status = 0x10,
  /// captive portal message
  cpm = 0x11,
  /// a gps or geo location (e.g longitude/latitude)
  location = 0x12,
  /// sensor captured data
  sensor = 0x13,
  /// an allert message that should be given immediate attention
  alert = 0x14,
  /// Max supported topics
  max_topics = 0xFF
};

enum reservedTopic {
  unused = 0x00,
  ping = 0x01,
  pong = 0x02,
  max_reserved = 0x0F
};

/*
0        07   12  19  15   20   25                 PO                    255
|        |    |   |   |    |    |                  |                     |
+--------+----+-+-+-+-+----+----+------------------+---------------------+
|  DUID  |MUID|T|P|D|H|DCRC|CTR |      DATA        | PATH (max 48 bytes) |
|        |    | |O|T|C|    |    | (max 184 bytes)  |                     |
+--------+----+-+-+-+-+----+----+------------------+---------------------+
DUID:      08  byte array          - Device Unique ID
MUID:      04  byte array          - Message unique ID
T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
PO  :      01  byte value          - Offset to the start of the
DT  :      01  byte value          - Duck Type
HC  :      01  byte value          - Hop count (the number of times the packet was relayed)
DCRC:      04  byte value          - Data section CRC
CTR :      04  byte value          - Frame counter
DATA:      192 byte array          - Data payload (e.g sensor read, text,...). Max is 188 bytes.
PATH:      048 byte array of DUIDs - Device UIDs having seen this packet. Max is 48 bytes (6 hops)
*/


class CdpPacket {
public:
  /// Device UID (8 bytes)
  std::vector<byte> duid;
  /// Message UID (4 bytes)
  std::vector<byte> muid;
  /// Message topic (1 byte)
  byte topic;
  /// Offset to the Path section (1 byte)
  byte path_offset;
  /// Reserved (2 bytes)
  std::vector<byte> reserved;
  /// crc32 for the data section
  uint32_t dcrc;
  /// Data section
  std::vector<byte> data;
  /// Path section (48 bytes max)
  std::vector<byte> path;

  CdpPacket(std::vector<byte> buffer) {
    int buffer_length = buffer.size();
    
    // get path start position
    int path_pos = buffer[PATH_OFFSET_POS];
    
    // duid
    duid.assign(&buffer[0], &buffer[DUID_LENGTH]);
    // muid
    muid.assign(&buffer[MUID_POS], &buffer[TOPIC_POS]);
    // topic
    topic = buffer[TOPIC_POS];
    // path offset
    path_offset = buffer[PATH_OFFSET_POS];
    // reserved
    reserved.assign(&buffer[RESERVED_POS], &buffer[DATA_CRC_POS]);
    // data crc
    dcrc = duckutils::toUnit32(&buffer[DATA_CRC_POS]);
    // data section
    data.assign(&buffer[DATA_POS], &buffer[path_pos]);
    // path section
    path.assign(&buffer[path_pos], &buffer[buffer_length]);
  }

  ~CdpPacket() {}

  void build(std::vector<byte> buffer) {
    int buffer_length = buffer.size();

    // get path start position
    int path_pos = buffer[PATH_OFFSET_POS];

    // duid
    duid.assign(&buffer[0], &buffer[DUID_LENGTH]);
    // muid
    muid.assign(&buffer[MUID_POS], &buffer[TOPIC_POS]);
    // topic
    topic = buffer[TOPIC_POS];
    // path offset
    path_offset = buffer[PATH_OFFSET_POS];
    // reserved
    reserved.assign(&buffer[RESERVED_POS], &buffer[DATA_CRC_POS]);
    // data crc
    dcrc = duckutils::toUnit32(&buffer[DATA_CRC_POS]);
    // data section
    data.assign(&buffer[DATA_POS], &buffer[path_pos]);
    // path section
    path.assign(&buffer[path_pos], &buffer[buffer_length]);
  }

  /**
   * @brief Get the path cdp packet section as hex string.
   *
   * @returns a hex string representing the path section of a cdp packet
   */
  String getPathAsHexString() {
    return duckutils::convertToHex(path.data(), path.size());
  }

  /**
   * @brief Resets the cdp packet and underlying byte buffers.
   *
   */
  void reset() {
    std::vector<byte>().swap(duid);
    std::vector<byte>().swap(muid);
    std::vector<byte>().swap(reserved);
    std::vector<byte>().swap(path);
    std::vector<byte>().swap(data);
    topic = 0;
    path_offset = 0;
    dcrc = 0;
  }
};

#endif