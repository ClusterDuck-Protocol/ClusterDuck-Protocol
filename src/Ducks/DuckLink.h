#ifndef DUCKLINK_H
#define DUCKLINK_H

#include "Duck.h"

template <typename RadioType = DuckLoRa>
class DuckLink : public Duck<RadioType> {
public:
using Duck<RadioType>::Duck;
  
  ~DuckLink() override {}
  
  /**
   * @brief Override the default setup method to match DuckLink specific
   * defaults.
   *
   * In addition to Serial component, the Radio component is also initialized.
   * When ssid and password are provided the duck will setup the wifi related
   * components.
   *
   * @param ssid wifi access point ssid (defaults to an empty string if not
   * provided)
   * @param password wifi password (defaults to an empty string if not provided)
   * 
   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
  int setupWithDefaults(std::array<byte,8> deviceId, std::string ssid="", std::string password="") {
    int err = Duck<RadioType>::setupWithDefaults(deviceId, ssid, password);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err);
      return err;
    }
  
    err = this->setupRadio();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err);
      return err;
    }
    
    loginfo_ln("DuckLink setup done");
    return DUCK_ERR_NONE;
  };


  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  int getType() { return DuckType::LINK; }

private:
  void handleReceivedPacket() {
    if (this->duckRadio->getReceiveFlag()){
      std::vector<uint8_t> data;
  
      loginfo_ln("====> handleReceivedPacket: START");
  
      int err = this->duckRadio->readReceivedData(&data);
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR failed to get data from DuckRadio. rc = %d",err);
        return;
      }
      logdbg_ln("Got data from radio, prepare for relay. size: %d",data.size());
        
      CdpPacket packet = CdpPacket(this->rxPacket->getBuffer());
  
      //Check if Duck is desitination for this packet
      if (duckutils::isEqual(BROADCAST_DUID, packet.dduid)) {
        switch(packet.topic) {
          case reservedTopic::rreq: {
              // check number of hops in packet header
              // send RREP if DUCKORIGIN is one hop away
              // if more than one hop, return
              if( packet.hopCount > 1) {
                loginfo_ln("RREQ received from %s. Hop count is %d. Not sending RREP.",
                           duckutils::toString(packet.data).c_str(), packet.hopCount);
                return;
              } else {
                  loginfo_ln("RREQ received from %s. Sending RREP!",
                               packet.sduid.data());
                  //Update the RREQ with the current DUID
                  ArduinoJson::JsonDocument rreqDoc, rrepDoc;
                  deserializeJson(rreqDoc, packet.data);
                  auto destination = rreqDoc["origin"].as<std::string>();
                  //not sure if RREP's origin and source should be the same
                  DuckPacket::RREP(destination,this->deviceId,this->deviceId);
                  loginfo_ln("handleReceivedPacket: Sending RREP");
                  //Serialize the updated RREQ packet
                  std::string strRREP;
                  serializeJson(rrepDoc, strRREP);
                  //Prepare the RREP packet for sending
                  this->txPacket->prepareForSending(&this->filter,duckutils::stringToArray<uint8_t,8>(this->deviceId),
                                              DuckType::LINK, reservedTopic::rrep,
                                              duckutils::stringToByteVector(strRREP));
                  err = this->duckRadio.sendData(this->txPacket->getBuffer());
                  if (err != DUCK_ERR_NONE) {
                      logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d", err);
                  } else {
                      loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                  }
              }
            break;
          }
          case reservedTopic::ping:
            loginfo_ln("PING received. Sending PONG!");
            err = this->sendPong();
            if (err != DUCK_ERR_NONE) {
              logerr_ln("ERROR failed to send pong message. rc = %d",err);
            }
          break;
        }
      } else if(duckutils::isEqual(this->duid, packet.dduid)) { //Target device check
          std::vector<uint8_t> dataPayload;
          byte num = 1;
        
        switch(packet.topic) {
          case reservedTopic::rreq: {
              // send RREP unconditionally
  
            loginfo_ln("RREQ received. Updating RREQ!");
            ArduinoJson::JsonDocument rreqDoc;
            deserializeJson(rreqDoc, packet.data);
            DuckPacket::UpdateRREQ(rreqDoc, this->duid);
            loginfo_ln("handleReceivedPacket: RREQ updated with current DUID: %s", this->duid);
            //Serialize the updated RREQ packet
            // std::string strRREQ;
            // serializeJson(rreqDoc, strRREQ);
            // rxPacket->getBuffer() = duckutils::stringToByteVector(strRREQ);
          break;
        }
          case reservedTopic::rrep: {
              // check if the RREP ORIGIN is the same as the current device
              // add last hop to the path to routing table
              // then return
          }
        case reservedTopic::ping:
          loginfo_ln("PING received. Sending PONG!");
          err = this->sendPong();
          if (err != DUCK_ERR_NONE) {
            logerr_ln("ERROR failed to send pong message. rc = %d",err);
          }
        break;
        }
  
      } else {
        err = this->duckRadio.relayPacket(this->rxPacket);
        if (err != DUCK_ERR_NONE) {
          logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
        } else {
          loginfo_ln("handleReceivedPacket: packet RELAY DONE");
        }
      }
  
    }
    this->rxPacket->reset();
  };
};

#endif
