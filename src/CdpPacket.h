#ifndef CDPPACKET_H_
#define CDPPACKET_H_

#include "Arduino.h"
#include "include/DuckUtils.h"
#include "DuckLogger.h"
#include "include/DuckTypes.h"
#include <string>
#include <array>

#define MAX_HOPS 6

// field/section length (in bytes)
#define PACKET_LENGTH 256
#define DUID_LENGTH 8
#define MUID_LENGTH 4
#define DATA_CRC_LENGTH 4
#define HEADER_LENGTH 27

// field/section offsets
#define SDUID_POS 0
#define DDUID_POS 8
#define MUID_POS 16
#define TOPIC_POS 20
//#define PATH_OFFSET_POS 21
#define DUCK_TYPE_POS 21
#define HOP_COUNT_POS 22
#define DATA_CRC_POS 23
#define DATA_POS HEADER_LENGTH // Data section starts immediately after header

#define RESERVED_LENGTH 2
//#define MAX_PATH_LENGTH (MAX_HOPS * DUID_LENGTH)
#define MAX_DATA_LENGTH (PACKET_LENGTH - HEADER_LENGTH)
//#define MAX_PATH_OFFSET (PACKET_LENGTH - DUID_LENGTH - 1)

/*
Data Section of a broadcast ack (max 229 bytes):
0 1      8    12...
| |      |    |
+-+------+----+-...
|N| DUID |MUID|
+-+------+----+-...

Maximum number of DUID/MUID pairs is:
  floor( (maximum data payload size - N) / (DUID size + MUID size) )
Which is:
  floor( (229 - 1) / 12 ) = 19

N:              1  byte                - Number of DUID/MUID pairs
Each DUID:     08  byte array          - A Device Unique ID
Each MUID:     04  byte array          - Message unique ID
*/
#define MAX_MUID_PER_ACK 19


//Available command IDs (N)
#define CMD_HEALTH 0
#define CMD_WIFI 1
#define CMD_CHANNEL 2

/*
Data Section of a duck command (max 229 bytes):
0 1    229...
| |     |
+-+-----+-...
|N| VAL
+-+-----+-...

N:              1  byte                - Command lookup value
VAL:          228  bytes               - Value to set for command

*/

// header + 1 hop + 1 byte data
#define MIN_PACKET_LENGTH (HEADER_LENGTH + 1)


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
  /// Device health status
  health = 0x15,
  // Send duck commands
  dcmd = 0x16,
  // MQ7 Gas Sensor
  mq7 = 0xEF,
  // GP2Y Dust Sensor
  gp2y = 0xFA,
  // bmp280
  bmp280 = 0xFB,
  // DHT11 sensor
  dht11 = 0xFC,
  // ir sensor
  pir = 0xFD,
  // bmp180 
  bmp180 = 0xFE,
  // Max supported topics
  max_topics = 0xFF
};


enum reservedTopic {
  unused = 0x00,
  ping = 0x01,
  pong = 0x02,
  gps = 0x03,
  ack = 0x04,
  cmd = 0x05,
  max_reserved = 0x0F
};

/*
|0       |8       |16  |20|21|22|23  |27                                   255|
|        |        |    |  |  |  |    |                                        |
+--------+--------+----+--+--+--+----+----------------------------------------+
| SDUID  | DDUID  |MUID|T |DT|HC|DCRC|                 DATA                   |
|        |        |    |  |  |  |    |            (max 229 bytes)             |
+--------+--------+----+--+--+--+----+----------------------------------------+

SDUID:     08  byte array          - Source Device Unique ID
DDUID:     08  byte array          - Destination Device Unique ID
MUID:      04  byte array          - Message unique ID
T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
DT  :      01  byte value          - Duck Type 
HC  :      01  byte value          - Hop count (the number of times the packet was relayed)
DCRC:      04  byte value          - Data section CRC
DATA:      229 byte array          - Data payload (e.g sensor read, text,...)
*/

typedef std::array<byte,8> Duid;
typedef std::array<byte,8> Muid;

class CdpPacket {
public:
  /// Source Device UID (8 bytes)
  std::array<byte,8> sduid;
  /// Destination Device UID (8 bytes)
  std::array<byte,8> dduid;
  /// Message UID (4 bytes)
  std::array<byte,8> muid;
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
  //std::array<byte,48> path;
  //time received
  unsigned long timeReceived;

  CdpPacket() {
    reset();
  }
  CdpPacket(const std::vector<byte> & buffer) {
      int buffer_length = buffer.size();
      // sduid
      std::copy(&buffer[SDUID_POS], &buffer[DDUID_POS], sduid.begin());
      // dduid
      std::copy(&buffer[DDUID_POS], &buffer[MUID_POS], dduid.begin());
      // muid
      std::copy(&buffer[MUID_POS], &buffer[TOPIC_POS], muid.begin());
      // topic
      topic = buffer[TOPIC_POS];
      // duckType
      duckType = buffer[DUCK_TYPE_POS];
      // hop count
      hopCount = buffer[HOP_COUNT_POS];
      // data crc
      dcrc = duckutils::toUint32(&buffer[DATA_CRC_POS]);
      // data section
      data.assign(&buffer[DATA_POS], &buffer[buffer_length]);

  }

  ~CdpPacket() {}

  /**
   * @brief Resets the cdp packet and underlying byte buffers.
   *
   */
  void reset() {
    std::array<byte,8>().swap(sduid);
    std::array<byte,8>().swap(muid);
    //std::array<byte,8>().swap(path);
    data.clear();
    duckType = DuckType::UNKNOWN;
    hopCount = 0;
    topic = 0;
    path_offset = 0;
    dcrc = 0;
  }

  static std::string topicToString(int topic) {
    switch (topic) {
      case topics::status:
        return "status";
      case topics::cpm:
        return "cpm";
      case topics::location:
        return "location";
      case topics::sensor:
        return "sensor";
      case topics::alert:
        return "alert";
      case topics::health:
        return "health";
      case topics::dcmd:
        return "dcmd";
      case topics::mq7:
        return "mq7";
      case topics::gp2y:
        return "gp2y";
      case topics::bmp280:
        return "bmp280";
      case topics::dht11:
        return "dht11";
      case topics::pir:
        return "pir";
      case topics::bmp180:
        return "bmp180";
      case reservedTopic::ping:
        return "ping";
      default:
        return "unknown";
    }
  }
};

#endif
