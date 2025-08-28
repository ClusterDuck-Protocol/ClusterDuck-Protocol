#include "CdpPacket.h"

int CdpPacket::prepareForSending() {
    uint8_t data_length = data.size();

    if ( data.empty() || data_length > MAX_DATA_LENGTH) {
        return DUCKPACKET_ERR_SIZE_INVALID;
    }

    loginfo_ln("packet format: DATA LENGTH: %d - TOPIC (%s)", data_length, topicToString(this->topic).c_str());

// ----- insert packet header  -----
    loginfo_ln("packet format: START");
    std::string printStr;
// source device uid
    buffer.insert(buffer.end(), this->sduid.begin(), this->sduid.end());

    std::string strDuid(this->sduid.begin(), this->sduid.end());
    logdbg_ln("SDuid:      %s", strDuid.c_str());

// destination device uid  
    if (this->dduid == BROADCAST_DUID){
    logdbg_ln("DDuid:     broadcast");
    } else{
    buffer.insert(buffer.end(), this->dduid.begin(), this->dduid.end());
    logdbg_ln("DDuid: %s", std::string(this->dduid.begin(), this->dduid.end()).c_str());
    }
    
    // message uid
    buffer.insert(buffer.end(), this->muid.begin(), this->muid.end());
    printStr.assign(this->muid.begin(), this->muid.end());
    logdbg_ln("Muid:      %s", printStr.c_str());

    // topic
    buffer.insert(buffer.end(), this->topic);
    logdbg_ln("Topic:     %s", CdpPacket::topicToString(this->topic).c_str());

    // duckType
    buffer.insert(buffer.end(), this->duckType);
    logdbg_ln("duck type: %s", std::to_string(this->duckType).c_str());

    // hop count
    buffer.insert(buffer.end(), this->hopCount);
    logdbg_ln("hop count: %s", std::to_string(this->hopCount).c_str());

    std::array<uint8_t,DATA_CRC_LENGTH> crc_bytes; //could this belong elsewhere? like in duckradio??
    uint32_t value = CRC32::calculate(buffer.data(), buffer.size());
    crc_bytes[0] = (value >> 24) & 0xFF;
    crc_bytes[1] = (value >> 16) & 0xFF;
    crc_bytes[2] = (value >> 8) & 0xFF;
    crc_bytes[3] = value & 0xFF;
    
    // data crc
    buffer.insert(buffer.end(), crc_bytes.begin(), crc_bytes.end());
    logdbg_ln("Data CRC:  %s", duckutils::arrayToHexString(crc_bytes).c_str());

    buffer.insert(buffer.end(), this->data.begin(), this->data.end());
    //convert the data to string for logging
    logdbg_ln("Data:      %s",printStr.assign(this->data.begin(), this->data.end()).c_str());
    
    buffer.shrink_to_fit();
    logdbg_ln("Built packet (HEX): %s", duckutils::convertToHex(buffer.data(), buffer.size()).c_str());

    return DUCK_ERR_NONE;
}

