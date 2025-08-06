#ifndef DUCKPACKET_H_
#define DUCKPACKET_H_

#include "CdpPacket.h"
#include "Arduino.h"
#include "utils/DuckUtils.h"
#include "include/cdpcfg.h"
#include "./routing/bloomfilter.h"
#include "ArduinoJson.h"
#include <CRC32.h>
#include <array>

/**
 * @brief Use this DUID to send to all PapaDucks
 * 
 */
static std::array<uint8_t,8> PAPADUCK_DUID = {0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};

/**
 * @brief Use this DUID to be received by every duck in the network
 * 
 */
static std::array<uint8_t,8> BROADCAST_DUID = {0xFF, 0xFF, 0xFF, 0xFF,
                                           0xFF, 0xFF, 0xFF, 0xFF};

/**
 * @brief Encapsulate the protocol packet attributes and operations
 * 
 */
class DuckPacket {

public:

    /**
     * @brief Construct a new Duck Packet object.
     * 
     */
    DuckPacket() {
        buffer.reserve(256);
    }

    /**
     * @brief Construct a new Duck Packet object.
     * 
     * @param duid a duck device unique id
     * @param filter a bloom filter
     */
        DuckPacket(std::array<uint8_t,8> duid)
        : duid(duid) {
            buffer.reserve(256);
            }

    ~DuckPacket() {}
    /**
     * @brief Set device Id.
     *
     * @param duid a duck device unique id
     */
    void setDeviceId(std::array<uint8_t,8> duid) { this->duid = duid; }

    /**
     * @brief Get device Id.
     *
     * @returns a duck device unique id
     */
    std::array<uint8_t,8> getDeviceId() { return this->duid; }
    /**
     * @brief Build a packet from the given topic and provided byte buffer.
     *
     * @param targetDevice the target device DUID to receive the message
     * @param topic a message topic
     * @param app_data a byte buffer that contains the packet data section
     * @returns DUCK_ERR_NONE if the operation was successful, otherwise an error code.
     */
    int prepareForSending(BloomFilter *filter, const std::array<uint8_t,8> targetDevice,
                          uint8_t duckType, uint8_t topic, std::vector<uint8_t> app_data);

    /**
     * @brief Update a received packet if it needs to be relayed in the mesh.
     * 
     * @param filter The bloom filter describing what packets have already been seen
     * @param dataBuffer buffer containing the packet data
     * @returns true if the packet needs to be relayed
     * @returns false if the packet does not need to be replayed
     */
    bool prepareForRelaying(BloomFilter *filter, std::vector<uint8_t> dataBuffer);
    
    /**
     * @brief Get the Cdp Packet byte array.
     * 
     * @returns a array of bytes representing the cdp packet
     */
    std::vector<uint8_t> getBuffer() { return buffer;}

    /**
     * @brief Resets the packet byte buffer.
     *
     */
    void reset() {
      buffer.clear();
    }

    uint8_t getTopic() { return buffer[TOPIC_POS]; }

/**
* @brief Build a RREQ packet.
*
*
* @param sourceDevice the source device DUID to send the message
* @returns a JSON Document representing the RREQ packet
*/
   static ArduinoJson::JsonDocument RREQ(std::string& targetDevice, std::string& sourceDevice);

/**
 * @brief Update a RREQ packet's path with the current node's DUID.
 *
 * @param rreq the RREQ packet to update
 * @param currentDevice the device DUID to of the current node
 * @returns a JSON Document representing the updated RREQ packet
 */
   static void UpdateRREQ(ArduinoJson::JsonDocument& rreq, std::string currentDevice);

/**
 * @brief Build a RREP packet.
 * @param targetDevice
 * @param sourceDevice
 * @param originDevice
 * @return
 */
   static ArduinoJson::JsonDocument RREP(std::string &targetDevice, std::string &sourceDevice,
                            std::string &originDevice);
    /**
     * @brief Update a RREP packet's path with the current node's DUID.
     * @param rrep
     * @param currentDevice
     */
   static void UpdateRREP(JsonDocument &rrep, std::array<uint8_t, 8> currentDevice);

   static std::string prepareRREP(std::array<uint8_t,8> &deviceId,CdpPacket &packet);

  private: 
    std::array<uint8_t,8> duid;
    std::vector<uint8_t> buffer;

    static void getUniqueMessageId(BloomFilter * filter, std::array<uint8_t,MUID_LENGTH> &message_id);

    /**
     * @brief Get the data section of the packet.
     *
     * @returns a vector of bytes representing the data section
     */
    std::vector<uint8_t> getDataSection() { return std::vector<uint8_t>(buffer.begin() + DATA_POS, buffer.end()); }
};

#endif

