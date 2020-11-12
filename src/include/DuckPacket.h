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
 * @brief Encapsulate Duck Packet attributes and operations
 * 
 */
class DuckPacket {

public:
    DuckPacket() {}
    //DuckPacket(String device_id) { this->deviceId = device_id;}

    DuckPacket(std::vector<byte> duid) { this->duid = duid; }

    ~DuckPacket() {}

    void setDeviceId(std::vector<byte> duid) { this->duid = duid; }

    int prepareForSending(byte topic, std::vector<byte> app_data);
    bool prepareForRelaying(std::vector<byte> duid, std::vector<byte> dataBuffer);
    
    byte* getDataByteBuffer() { return buffer.data(); }
    
    std::vector<byte> getCdpPacketBuffer() { return buffer;}
    CDP_Packet getCdpPacket() { return packet; }

    int getBufferLength() { return buffer.size(); }
    
    
    bool relay(std::vector<byte> duid);

    String getPathAsHexString() {
      return duckutils::convertToHex(packet.path.data(), packet.path.size());
    }
    
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

