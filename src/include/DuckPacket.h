#ifndef DUCKPACKET_H_
#define DUCKPACKET_H_
#include "Arduino.h"
#include "cdpcfg.h"
#include <WString.h>
#include <array>

#define MAX_HOPS        6

// PACKET FORMAT (v1.0)
//
// 0       07          15                       PO                   255
// |        |           |                       |                     |
// +--------+----+-+-+--+-----------------------+---------------------+
// |  DUID  |MUID|T|P| R| DATA (max 192 bytes)  | PATH (max 48 bytes) |
// |        |    | |O|  |                       |                     |
// +--------+----+-+-+--+-----------------------+---------------------+
//
// DUID:      08  byte array          - Device Unique ID    
// MUID:      04  byte array          - Message unique ID
// T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
// PO  :      02  byte array          - Offset to the start of the PATH section in the packet 
// R   :      02  byte array          - Reserved for internal use
// DATA:      192 byte array          - Data payload (e.g sensor read, text,...)
// PATH:      048 byte array of DUIDs - Device UIDs having seen this packet    

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
#define DATA_OFFSET_POS     HEADER_LENGTH
#define PATH_POS            208

#define RESERVED_LENGTH 2

enum resevedTopic {
  health    = 0x00,
  ping      = 0x01,
  pong      = 0x02,
  gps       = 0x03,
  max_topic = 0x0F
};

class DuckPacket {

public:
    DuckPacket(String device_id) {
      this->deviceId = device_id;
    }
    ~DuckPacket() {}

    void setDeviceId(String device_id) { this->deviceId = device_id; }
    int buildDataBuffer(byte topic, byte* app_data);
    
    byte* getBuffer() {
      return buffer.data();
    }

    int getBufferLength() {
      return buffer.size();
    }
    
    int updatePath();

private:
  String deviceId;
  std::vector<byte> buffer;
  int bufferLength;
};

#endif

