#ifndef DUCKPACKET_H_
#define DUCKPACKET_H_

#include "../CdpPacket.h"
#include "Arduino.h"
#include "DuckUtils.h"
#include "cdpcfg.h"
#include <CRC32.h>
#include <WString.h>
#include <vector>

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
     * @param topic a message topic
     * @param app_data a byte buffer that contains the packet data section
     * @returns DUCK_ERR_NONE if the operation was successful, otherwise an error code.
     */
    int prepareForSending(byte topic, std::vector<byte> app_data);

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
     * @brief Get the Data Byte Buffer from a packet.
     * 
     * @returns a byte array representing the packet
     */
    byte* getDataByteBuffer() { return buffer.data(); }
    
    /**
     * @brief Get the Cdp Packet byte vector.
     * 
     * @returns a vector of bytes representing the cdp packet 
     */
    std::vector<byte> getCdpPacketBuffer() { return buffer;}
    /**
     * @brief Get the Cdp Packet object
     * 
     * @returns a cdp packet 
     */
    CDP_Packet getCdpPacket() { return packet; }

    /**
     * @brief Get the CDP Packet buffer length.
     * 
     * @returns length in bytes of the cdp packet buffer.
     */
    int getBufferLength() { return buffer.size(); }
    
    /**
     * @brief Checks if a packet needs to be relayed and update the path section.
     * of the cdp packet
     * 
     * @param duid the device uid to add to the path section if packet is relayed
     * @returns true if packet should be relayed.
     * @returns false if packet should not be relayed.
     */
    bool relay(std::vector<byte> duid);

    /**
     * @brief Get the path cdp packet section as hex string object.
     * 
     * @returns a hex string representing the path section of a cdp packet 
     */
    String getPathAsHexString() {
      return duckutils::convertToHex(packet.path.data(), packet.path.size());
    }
    /**
     * @brief Resets the cdp packet and underlying byte buffers.
     * 
     */
    void reset() {
      std::vector<byte>().swap(packet.duid);
      std::vector<byte>().swap(packet.muid);
      std::vector<byte>().swap(packet.reserved);
      std::vector<byte>().swap(packet.path);
      std::vector<byte>().swap(packet.data);
      std::vector<byte>().swap(buffer);
      packet.topic = 0;
      packet.path_offset = 0;
      packet.dcrc = 0;
    }

private : 
    std::vector<byte> duid;
    std::vector<byte> buffer;
    CDP_Packet packet;
};

#endif

