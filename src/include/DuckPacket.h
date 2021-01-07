#ifndef DUCKPACKET_H_
#define DUCKPACKET_H_

#include "../CdpPacket.h"
#include "Arduino.h"
#include "DuckUtils.h"
#include "cdpcfg.h"
#include <CRC32.h>
#include <WString.h>
#include <vector>

static std::vector<byte> ZERO_DUID = {0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};

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
    DuckPacket() {}
    /**
     * @brief Construct a new Duck Packet object.
     * 
     * @param duid a duck device unique id
     */
    DuckPacket(std::vector<byte> duid) { this->duid = duid; }

    ~DuckPacket() {}
    /**
     * @brief Set device Id.
     *
     * @param duid a duck device unique id
     */
    void setDeviceId(std::vector<byte> duid) { this->duid = duid; }

    /**
     * @brief Build a packet from the given topic and provided byte buffer.
     *
     * @param targetDevice the target device to receive the message
     * @param topic a message topic
     * @param app_data a byte buffer that contains the packet data section
     * @returns DUCK_ERR_NONE if the operation was successful, otherwise an error code.
     */
    int prepareForSending(const std::vector<byte> targetDevice, byte duckType, byte topic, std::vector<byte> app_data);

    /**
     * @brief Update a received packet if it needs to be relayed in the mesh.
     * 
     * @param duid unique Id of the device relaying the packet
     * @param dataBuffer buffer containing the packet data
     * @returns true if the packet needs to be relayed
     * @returns false if the packet does not need to be replayed
     */
    bool prepareForRelaying(std::vector<byte> duid, std::vector<byte> dataBuffer);
    
    /**
     * @brief Get the Cdp Packet byte vector.
     * 
     * @returns a vector of bytes representing the cdp packet 
     */
    std::vector<byte> getBuffer() { return buffer;}
    
    /**
     * @brief Checks if a packet needs to be relayed and update the path section.
     * of the cdp packet
     * 
     * @param duid the device uid to add to the path section if packet is relayed
     * @param path_section the path section of the cdp packet to check
     * @returns true if packet should be relayed.
     * @returns false if packet should not be relayed.
     */
    bool relay(std::vector<byte> duid, std::vector<byte> path_section);

    /**
     * @brief Resets the packet byte buffer.
     *
     */
    void reset() {
      std::vector<byte>().swap(buffer);
    }

    byte getTopic() { return buffer[TOPIC_POS]; }


  private: 
    std::vector<byte> duid;
    std::vector<byte> buffer;
};

#endif

