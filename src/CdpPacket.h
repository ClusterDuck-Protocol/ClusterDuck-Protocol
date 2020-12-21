#ifndef CDPPACKET_H_
#define CDPPACKET_H_

#include "Arduino.h"
#include "include/DuckUtils.h"
#include "DuckLogger.h"
#include "include/DuckTypes.h"
#include <string>
#include <vector>

#define MAX_HOPS 6

// field/section length (in bytes)
#define PACKET_LENGTH 256
#define DUID_LENGTH 8
#define MUID_LENGTH 4
#define DATA_CRC_LENGTH 4
#define HEADER_LENGTH 28

// field/section offsets
#define SDUID_POS 0
#define DDUID_POS 8
#define MUID_POS 16
#define TOPIC_POS 20
#define PATH_OFFSET_POS 21
#define DUCK_TYPE_POS 22
#define HOP_COUNT_POS 23
#define DATA_CRC_POS 24
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
  gps = 0x03,
  max_reserved = 0x0F
};

/*
0        7       15   19      23   27                  PO                  255
|        |        |    | | | | |    |                  |                     |
+--------+--------+----+-+-+-+-+----+------------------+---------------------+
| SDUID  | DDUID  |MUID|T|P|D|H|DCRC|      DATA        | PATH (max 48 bytes) |
|        |        |    | |O|T|C|    | (max 180 bytes)  |                     |
+--------+--------+----+-+-+-+-+----+------------------+---------------------+

SDUID:     08  byte array          - Source Device Unique ID
DDUID:     08  byte array          - Destination Device Unique ID
MUID:      04  byte array          - Message unique ID
T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
PO  :      01  byte value          - Offset to the start of the Path section
DT  :      01  byte value          - Duck Type 
HC  :      01  byte value          - Hop count (the number of times the packet was relayed)
DCRC:      04  byte value          - Data section CRC
DATA:      188 byte array          - Data payload (e.g sensor read, text,...)
PATH:      048 byte array of DUIDs - Device UIDs having seen this packet - Max is 48 bytes (6 hops)
*/

class CdpPacket {
public:
  /// Source Device UID (8 bytes)
  std::vector<byte> sduid;
  /// Destination Device UID (8 bytes)
  std::vector<byte> dduid;
  /// Message UID (4 bytes)
  std::vector<byte> muid;
  /// Message topic (1 byte)
  byte topic;
  /// Offset to the Path section (1 byte)
  byte path_offset;
  /// Type of ducks as define in DuckTypes.h
  byte duckType;
  /// Number of times a packet was relayed in the mesh
  byte hopCount;
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
    
    // sduid
    sduid.assign(&buffer[SDUID_POS], &buffer[DDUID_POS]);
    // dduid
    dduid.assign(&buffer[DDUID_POS], &buffer[MUID_POS]);
    // muid
    muid.assign(&buffer[MUID_POS], &buffer[TOPIC_POS]);
    // topic
    topic = buffer[TOPIC_POS];
    // path offset
    path_offset = buffer[PATH_OFFSET_POS];
    // duckType
    duckType = buffer[DUCK_TYPE_POS];
    // hop count
    hopCount = buffer[HOP_COUNT_POS];
    // data crc
    dcrc = duckutils::toUnit32(&buffer[DATA_CRC_POS]);
    // data section
    data.assign(&buffer[DATA_POS], &buffer[path_pos]);
    // path section
    path.assign(&buffer[path_pos], &buffer[buffer_length]);
  }

  ~CdpPacket() {}


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
    std::vector<byte>().swap(sduid);
    std::vector<byte>().swap(muid);
    std::vector<byte>().swap(path);
    std::vector<byte>().swap(data);
    duckType = DuckType::UNKNOWN;
    hopCount = 0;
    topic = 0;
    path_offset = 0;
    dcrc = 0;
  }
};

#endif