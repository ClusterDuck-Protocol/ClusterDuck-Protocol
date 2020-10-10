#ifndef DUCKPACKET_H_
#define DUCKPACKET_H_

#include "../CdpPacket.h"
#include "Arduino.h"
#include "DuckUtils.h"
#include "cdpcfg.h"
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

    void setDeviceId(String device_id) { this->deviceId = device_id; }
    void setDeviceId(std::vector<byte> duid) { this->duid = duid; }

    int buildPacketBuffer(byte topic, std::vector<byte> app_data);
    bool update(std::vector<byte> duid, std::vector<byte> dataBuffer);
    
    byte* getDataByteBuffer() { return buffer.data(); }
    
    std::vector<byte> getCdpPacketBuffer() { return buffer;}
    CDP_Packet getCdpPacket() { return packet; }

    int getBufferLength() { return buffer.size(); }
    
    
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

