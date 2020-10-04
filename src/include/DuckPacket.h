#ifndef DUCKPACKET_H_
#define DUCKPACKET_H_
#include <vector>
#include "Arduino.h"
#include "cdpcfg.h"
#include <WString.h>
#include "DuckUtils.h"


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
// T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
// PO  :      01  byte array          - Offset to the start of the PATH section in the packet 
// R   :      02  byte array          - Reserved for internal use
// DATA:      192 byte array          - Data payload (e.g sensor read, text,...)
// PATH:      048 byte array of DUIDs - Device UIDs having seen this packet    

#define MAX_HOPS        6

// field/section length (in bytes)
#define PACKET_LENGTH   256
#define DUID_LENGTH     8
#define MUID_LENGTH     4
#define HEADER_LENGTH   16
#define MAX_PATH_LENGTH (MAX_HOPS * MUID_LENGTH)
#define MAX_DATA_LENGTH (PACKET_LENGTH - HEADER_LENGTH - MAX_PATH_LENGTH)

// field/section offsets
#define DUID_POS            0
#define MUID_POS            8
#define TOPIC_POS           12
#define PATH_OFFSET_POS     13
#define RESERVED_POS        14
#define DATA_POS            HEADER_LENGTH // Data section starts immediately after header

#define RESERVED_LENGTH 2

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

/**
 * @brief Defines preset topic tags for duck packets.
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
  /// Max supported topic tags
  max_topics = 0xFF
};

/**
 * @brief Duck packet format.
 *
 * PACKET FORMAT (v1.0)
 * 0        07   12     15                      PO                    255
 * |        |    |      |                       |                     |
 * +--------+----+-+-+--+-----------------------+---------------------+
 * |  DUID  |MUID|T|P| R| DATA (max 192 bytes)  | PATH (max 48 bytes) |
 * |        |    | |O|  |                       |                     |
 * +--------+----+-+-+--+-----------------------+---------------------+
 *
 * DUID:      08  byte array          - Device Unique ID
 * MUID:      04  byte array          - Message unique ID
 * T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
 * PO  :      01  byte array          - Offset to the start of the PATH section in the packet
 * R   :      02  byte array          - Reserved for internal use
 * DATA:      192 byte array          - Data payload (e.g sensor read, text,...). Max is 192 bytes.
 * PATH:      048 byte array of DUIDs - Device UIDs having seen this packet. Max is 48 bytes (6 hops)
 *
 */
#pragma pack(1)
typedef struct {
  std::vector<byte> duid;
  std::vector<byte> muid;
  byte topic;
  byte path_offset;
  std::vector<byte> reserved;
  std::vector<byte> data;
  std::vector<byte> path;
} CDP_Packet;

/**
 * @brief Encapsulate Duck Packet attributes and operations
 * 
 */
class DuckPacket {

public:
    DuckPacket() {}
    DuckPacket(String device_id) { this->deviceId = device_id;}

    DuckPacket(std::vector<byte> duid) { this->duid = duid; }

    ~DuckPacket() {}

    void setDeviceId(String device_id) { this->deviceId = device_id; }
    void setDeviceId(std::vector<byte> duid) { this->duid = duid; }

    int buildPacketBuffer(byte topic, std::vector<byte> app_data);
    bool update(std::vector<byte> duid, std::vector<byte> dataBuffer);

    byte* getDataByteBuffer() { return buffer.data(); }

    std::vector<byte> getCdpPacketBuffer() { return buffer;}
    CDP_Packet getCdpPacket() { return packet; }

    int getBufferLength() { return buffer.size(); }
    
    bool hasMaxHops();

    int updatePath();
    bool relay(std::vector<byte> duid);
    String getPathAsHexString() {
      return duckutils::convertToHex(packet.path.data(), packet.path.size());
    }
    void reset() {
      packet.duid.clear();
      packet.muid.clear();
      packet.topic = 0;
      packet.path_offset = 0;
      packet.reserved.clear();
      packet.path.clear();
      packet.data.clear();
      buffer.clear();
    }

    private : 
    String deviceId;
    std::vector<byte> duid;
    std::vector<byte> buffer;
    CDP_Packet packet;
    int bufferLength;
};

#endif

