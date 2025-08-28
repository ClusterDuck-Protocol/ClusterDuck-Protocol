#ifndef DUCKLINK_H
#define DUCKLINK_H

#include "Duck.h"

template <typename WifiCapability, typename RadioType = DuckLoRa>
class DuckLink : public Duck<WifiCapability, RadioType> {
public:
using Duck<WifiCapability, RadioType>::Duck;
  
  DuckLink(std::string name = "LINK0001") : Duck<WifiCapability, RadioType>(std::move(name)) {}
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
    int err = Duck<WifiCapability, RadioType>::setupWithDefaults(deviceId, ssid, password);
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
        
      // CdpPacket packet = CdpPacket(this->rxPacket->rawBuffer());
      CdpPacket packet = new CdpPacket;
  
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
                // this->txPacket->prepareForSending(&this->filter, duckutils::stringToArray<uint8_t,8>(this->deviceId),
                //                             DuckType::LINK, reservedTopic::rrep,
                //                             duckutils::stringToByteVector(CdpPacket::prepareRREP(this->deviceId, packet)));
                // err = this->duckRadio.sendData(this->txPacket->rawBuffer());
                // if (err != DUCK_ERR_NONE) {
                //     logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
                // } else {
                    loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
                // }
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

            // this->txPacket->prepareForSending(&this->filter, this-> deviceId ,DuckType::LINK, reservedTopic::rrep, duckutils::stringToByteVector(CdpPacket::prepareRREP(this->duid, packet)));
            // err = this->duckRadio.sendData(this->txPacket->rawBuffer());
            // if (err != DUCK_ERR_NONE) {
            //     logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
            // } else {
                loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
            // }
          break;
        }
        case reservedTopic::rrep: {
          // check if the RREP ORIGIN is the same as the current deviceId
          JsonDocument rrepDoc;
          deserializeJson(rrepDoc, packet.data);
          std::string origin = rrepDoc["origin"].as<std::string>();
          ArduinoJson::JsonArray path = rrepDoc["path"].to<ArduinoJson::JsonArray>();
          if( origin == duckutils::toString(this->duid)) {
            loginfo_ln("RREP received from %s. This is the origin device. Adding last hop to routing table.",
                       duckutils::toString(packet.data).c_str());
              ArduinoJson::JsonArray path = rrepDoc["path"].to<ArduinoJson::JsonArray>();
              // add last hop to the routing table
              auto lastHop = path[path.size() - 1].as<std::string>();
              this->insertIntoRoutingTable(lastHop, 0, millis(), this->duckRadio.getSNR(), this->duckRadio.getRSSI());
              loginfo_ln("Last hop: %s", lastHop.c_str());
            return;
          } else {

            //update route response path by deleting last node in path
              path.remove(path.end());
              rrepDoc["path"] = path;
              // serialize the updated RREP packet
              std::string strRREP;
              serializeJson(rrepDoc, strRREP);
              auto destinationDUID = path.end()->as<std::string>();
              // this->txPacket->prepareForSending(&this->filter, duckutils::stringToArray<uint8_t,8>(destinationDUID),
              //                             DuckType::LINK, reservedTopic::rrep,
              //                             duckutils::stringToByteVector(strRREP));
              // err = this->duckRadio.sendData(this->txPacket->rawBuffer());
              // if (err != DUCK_ERR_NONE) {
              //     logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
              // } else {
              //     loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
              // }
          }
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
        // err = this->duckRadio.relayPacket(this->rxPacket);
        if (err != DUCK_ERR_NONE) {
          logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
        } else {
          loginfo_ln("handleReceivedPacket: packet RELAY DONE");
        }
      }
  
    }
  };
};

#endif
