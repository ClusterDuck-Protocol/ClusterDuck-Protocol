#ifndef CDPPACKET_H
#define CDPPACKET_H

#include "Arduino.h"
#include <WString.h>
#include <vector>

// PACKET FORMAT (v1.0)
//
// 0        07   12     15                      PO                    255
// |        |    |      |                       |                     |
// +--------+----+-+-+--+-----------------------+---------------------+
// |  DUID  |MUID|T|P| R| DATA (max 192 bytes)  | PATH (max 48 bytes) |
// |        |    | |O|  |                       |                     |
// +--------+----+-+-+--+-----------------------+---------------------+
//
// DUID:      08  byte array          - Device Unique ID
// MUID:      04  byte array          - Message unique ID
// T   :      01  byte value          - Topic (topic 0..15 are reserved for
// internal use) PO  :      01  byte array          - Offset to the start of the
// PATH section in the packet R   :      02  byte array          - Reserved for
// internal use DATA:      192 byte array          - Data payload (e.g sensor
// read, text,...) PATH:      048 byte array of DUIDs - Device UIDs having seen
// this packet

#define MAX_HOPS 6

// field/section length (in bytes)
#define PACKET_LENGTH 256
#define DUID_LENGTH 8
#define MUID_LENGTH 4
#define HEADER_LENGTH 16
#define MAX_PATH_LENGTH (MAX_HOPS * MUID_LENGTH)
#define MAX_DATA_LENGTH (PACKET_LENGTH - HEADER_LENGTH - MAX_PATH_LENGTH)
#define MAX_PATH_OFFSET (PACKET_LENGTH - DUID_LENGTH - 1)

// 16 bytes header + 1 hop + 1 byte data
#define MIN_PACKET_LENGTH (HEADER_LENGTH + DUID_LENGTH + 1)

// field/section offsets
#define DUID_POS 0
#define MUID_POS 8
#define TOPIC_POS 12
#define PATH_OFFSET_POS 13
#define RESERVED_POS 14
#define DATA_POS HEADER_LENGTH // Data section starts immediately after header

#define RESERVED_LENGTH 2

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


/* PACKET FORMAT (v1.0)
 * 0        07   12     15                      PO                    255
 * |        |    |      |                       |                     |
 * +--------+----+-+-+--+-----------------------+---------------------+
 * |  DUID  |MUID|T|P| R| DATA (max 192 bytes)  | PATH (max 48 bytes) |
 * |        |    | |O|  |                       |                     |
 * +--------+----+-+-+--+-----------------------+---------------------+
 *
 * DUID:      08  byte array          - Device Unique ID
 * MUID:      04  byte array          - Message unique ID
 * T   :      01  byte value          - Topic (topic 0..15 are reserved for
 * internal use) PO  :      01  byte array          - Offset to the start of the
 * PATH section in the packet R   :      02  byte array          - Reserved for
 * internal use DATA:      192 byte array          - Data payload (e.g sensor
 * read, text,...). Max is 192 bytes. PATH:      048 byte array of DUIDs -
 * Device UIDs having seen this packet. Max is 48 bytes (6 hops)
 */
typedef struct {
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
  /// Data section (192 bytes max)
  std::vector<byte> data;
  /// Path section (48 bytes ma)
  std::vector<byte> path;
} CDP_Packet;

#endif