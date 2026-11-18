#ifndef CDPPACKET_H
#define CDPPACKET_H

#include "Arduino.h"
#include "utils/DuckUtils.h"
#include "utils/DuckLogger.h"
#include "utils/DuckError.h"
#include "Ducks/DuckTypes.h"
#include <string>
#include <array>
#include <CRC32.h>
#include "routing/SignalScore.h"
#include <optional>

#define MAX_HOPS 6

// field/section length (in bytes)
// SX126x/SX127x LoRa frames carry at most 255 bytes, including this header.
#define PACKET_LENGTH 255
#define DUID_LENGTH 8
#define MUID_LENGTH 4
#define DATA_CRC_LENGTH 4
#define HEADER_LENGTH 27

// field/section offsets
#define SDUID_POS 0
#define DDUID_POS 8
#define MUID_POS 16
#define TOPIC_POS 20
#define DUCK_TYPE_POS 21
#define HOP_COUNT_POS 22
#define DATA_CRC_POS 23
#define DATA_POS HEADER_LENGTH // Data section starts immediately after header

#define RESERVED_LENGTH 2
//#define MAX_PATH_LENGTH (MAX_HOPS * DUID_LENGTH)
#define MAX_DATA_LENGTH (PACKET_LENGTH - HEADER_LENGTH)

/*
Data Section of a broadcast ack (max 228 bytes):
0 1      8    12...
| |      |    |
+-+------+----+-...
|N| DUID |MUID|
+-+------+----+-...

Maximum number of DUID/MUID pairs is:
  floor( (maximum data payload size - N) / (DUID size + MUID size) )
Which is:
  floor( (228 - 1) / 12 ) = 18

N:              1  byte                - Number of DUID/MUID pairs
Each DUID:     08  byte array          - A Device Unique ID
Each MUID:     04  byte array          - Message unique ID
*/
#define MAX_MUID_PER_ACK ((MAX_DATA_LENGTH - 1) / (DUID_LENGTH + MUID_LENGTH))


//Available command IDs (N)
#define CMD_HEALTH 0
#define CMD_WIFI 1
#define CMD_CHANNEL 2

/*
Data Section of a duck command (max 228 bytes):
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
//TODO: make these extensible by plugins
enum topics {
  /// generic message (e.g non emergency messages)
  status = 0x10,
  /// captive portal message
  cpm = 0x11,
  /// sensor captured data
  sensor = 0x13,
  /// an allert message that should be given immediate attention
  alert = 0x14,
  /// Device health status
  health = 0x15,
  /// Deprecated: no longer handled by CDP. Kept so existing sketches compile.
  dcmd = 0x16,
  //node signal data
  sig = 0x17,
  //battery level
  bat = 0x18,
  //duck command for spreading factor and tx power
  cmd_tx = 0x20,
  //UTC time from a PapaDuck, {"epoch":<seconds>}; see PapaDuck::sendTimeSync()
  cmd_time = 0x22,
  //gps
  gps = 0xEA,
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
  // SHT40 temperature & humidity sensor
  sht40 = 0xF9,
  // Max supported topics
  max_topics = 0xFF
};

//internal protocol topics
enum reservedTopic {
  unused = 0x00,
  ping = 0x01,
  pong = 0x02,
  ack = 0x04,
  /// Deprecated: relayed like any other packet. Kept so existing sketches compile.
  cmd = 0x05,
  rreq = 0x06,
  rrep = 0x07,
  max_reserved = 0x0F
};

/**
 * @brief Use this DUID to send to all PapaDucks
 * 
 */
static std::array<uint8_t,8> PAPADUCK_DUID = {0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

/**
 * @brief Printable stand-in for PAPADUCK_DUID ("11111111")
 *
 * Route JSON carries DUIDs as text, and PAPADUCK_DUID's zero bytes cannot be
 * written there, so route JSON uses this alias instead (as broadcast uses "*").
 * Phoenix 2026 firmware also used it as the PapaDuck header address; ducks
 * still accept that and treat it as PAPADUCK_DUID.
 */
static std::array<uint8_t,8> PAPADUCK_ALIAS_DUID = {0x31, 0x31, 0x31, 0x31,
    0x31, 0x31, 0x31, 0x31};
/**
* @brief Use this DUID to be received by every duck in the network
* 
*/
static std::array<uint8_t,8> BROADCAST_DUID = {0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF};


/*
|0       |8       |16  |20|21|22|23  |27                                   255|
|        |        |    |  |  |  |    |                                        |
+--------+--------+----+--+--+--+----+----------------------------------------+
| SDUID  | DDUID  |MUID|T |DT|HC|DCRC|                 DATA                   |
|        |        |    |  |  |  |    |            (max 228 bytes)             |
+--------+--------+----+--+--+--+----+----------------------------------------+

SDUID:     08  byte array          - Source Device Unique ID
DDUID:     08  byte array          - Destination Device Unique ID
MUID:      04  byte array          - Message unique ID
T   :      01  byte value          - Topic (topic 0..15 are reserved for internal use)
DT  :      01  byte value          - Duck Type 
HC  :      01  byte value          - Hop count (the number of times the packet was relayed)
DCRC:      04  byte value          - Data section CRC
DATA:      228 byte array          - Data payload (e.g sensor read, text,...)
*/

typedef std::array<uint8_t,8> Duid;
typedef std::array<uint8_t,4> Muid;

class CdpPacket {
    public:
        CdpPacket() {
            sduid.fill(0);
            dduid.fill(0);
            muid.fill(0);
            topic = 0;
            duckType = DuckType::UNKNOWN;
            hopCount = 0;
            dcrc = 0;
            timeReceived = 0;
            buffer.reserve(256);
        }

        /**
         * @brief Create CdpPacket from rx buffer
         */
        CdpPacket(std::vector<uint8_t> rxBuffer) {
            buffer = rxBuffer;
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
            //need to figure out how to deal with timeReceived
        }

        /**
         * @brief Create CdpPacket for new tx data
         */
        CdpPacket(Duid dduid, uint8_t topic, const std::vector<uint8_t> data, Duid sduid, DuckType duckType){
            // Initialize default values
            this->sduid = sduid;
            this->dduid = dduid;
            this->topic = topic;
            this->duckType = duckType;
            this->hopCount = 0;
            this->timeReceived = -1;
            this->data = data;
            buffer.reserve(256);

        }

        ~CdpPacket() {}

        /// Source Device UID (8 bytes)
        std::array<uint8_t,8> sduid;
        /// Destination Device UID (8 bytes)
        std::array<uint8_t,8> dduid;
        /// Message UID (4 bytes)
        std::array<uint8_t,4> muid;
        /// Message topic (1 byte)
        uint8_t topic;
        /// Type of ducks as define in DuckTypes.h
        uint8_t duckType;
        /// Number of times a packet was relayed in the mesh
        uint8_t hopCount;
        /// crc32 for the data section
        uint32_t dcrc;
        /// Data section
        std::vector<uint8_t> data;
        //time received
        unsigned long timeReceived;
        // Local RX metadata only; never serialized into the packet header.
        std::optional<SignalScore> receivedSignal;


        /**
         * @brief Format packet to byte array and add crc
         *
         * @returns DUCK_ERR_NONE if the operation was successful, otherwise an error code.
         */
        int prepareForSending();

        /**
         * @brief Get the Cdp Packet as a raw byte array.
         * 
         * @returns a array of bytes representing the cdp packet
         */
        std::vector<uint8_t> asBytes() { return this->buffer;}

        int size(){
            //if buffer init return buffer size
            return this->buffer.size();
        }

        /**
         * @brief Convert packet topic to std::string
         * 
         * @returns string representing the topic
         */
        std::string topicToString() {
            switch (this->topic) {
            case reservedTopic::rreq:
                return "rreq";
            case reservedTopic::rrep:
                return "rrep";
            case topics::status:
                return "status";
            case topics::cpm:
                return "cpm";
            case topics::gps:
                return "gps";
            case topics::sensor:
                return "sensor";
            case topics::bat:
                return "battery";
            case topics::sig:
                return "signal health";
            case topics::alert:
                return "alert";
            case topics::health:
                return "health";
            case topics::dcmd:
                return "dcmd";
            case topics::cmd_time:
                return "cmd_time";
            case topics::cmd_tx:
                return "cmd_tx";
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
            case topics::sht40:
                return "sht40";
            case reservedTopic::ping:
                return "ping";
            case reservedTopic::pong:
                return "pong";
            default:
                return "unknown";
            }
        }

    private:
        std::vector<uint8_t> buffer;
};

#endif
