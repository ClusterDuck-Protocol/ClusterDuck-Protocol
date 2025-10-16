#ifndef DUCKLINK_H
#define DUCKLINK_H

#include "Duck.h"

template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class DuckLink : public Duck<WifiCapability, RadioType> {
  public:
    using Duck<WifiCapability, RadioType>::Duck;
    
    DuckLink(std::string name = "LINK0001") : Duck<WifiCapability, RadioType>(std::move(name)) {}
    ~DuckLink() override {}
    
    /**
     * @brief Get the DuckType
     *
     * @returns the duck type defined as DuckType
     */
    DuckType getType() { return DuckType::LINK; }

  private:
    void handleReceivedPacket() {
      if (this->duckRadio.getReceiveFlag()){
        std::vector<uint8_t> data;
    
        loginfo_ln("====> handleReceivedPacket: START");
    
        int err;
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData) {
          logerr_ln("ERROR failed to get data from DuckRadio.");
          return;
        }
        CdpPacket rxPacket(rxData.value());
        // Update routing table with signal info from last received packet
        this->router.insertIntoRoutingTable(rxPacket.sduid, this->getSignalScore(), millis());

        logdbg_ln("Got data from radio, prepare for relay. size: %d",rxPacket.size());
    
        //Check if Duck is desitination for this packet
        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
          switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                // check number of hops in packet header
                // send RREP if DUCKORIGIN is one hop away
                // if more than one hop, return
                if( rxPacket.hopCount > 1) {
                  loginfo_ln("RREQ received from %s. Hop count is %d. Not sending RREP.",
                            duckutils::toString(rxPacket.data).c_str(), rxPacket.hopCount);
                  return;
                } else {
                  // this->txPacket->prepareForSending(&this->filter, duckutils::stringToArray<uint8_t,8>(this->deviceId),
                  //                             DuckType::LINK, reservedTopic::rrep,
                  //                             duckutils::stringToByteVector(CdpPacket::prepareRREP(this->deviceId, packet)));
                  // err = this->duckRadio.sendData(this->txPacket->asBytes());
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
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            std::vector<uint8_t> dataPayload;
            byte num = 1;
          
          switch(rxPacket.topic) {
            case reservedTopic::rreq: {
              // send RREP unconditionally

              // this->txPacket->prepareForSending(&this->filter, this-> deviceId ,DuckType::LINK, reservedTopic::rrep, duckutils::stringToByteVector(CdpPacket::prepareRREP(this->duid, packet)));
              // err = this->duckRadio.sendData(this->txPacket->asBytes());
              // if (err != DUCK_ERR_NONE) {
              //     logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
              // } else {
                  loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
              // }
            break;
          }
          case reservedTopic::rrep: {
            // check if the RREP ORIGIN is the same as the current deviceId
            // JsonDocument rrepDoc;
            // deserializeJson(rrepDoc, rxPacket.data);
            // std::string origin = rrepDoc["origin"].as<std::string>();
            // ArduinoJson::JsonArray path = rrepDoc["path"].to<ArduinoJson::JsonArray>();
            // if( origin == duckutils::toString(this->duid)) {
            //   loginfo_ln("RREP received from %s. This is the origin device. Adding last hop to routing table.",
            //             duckutils::toString(rxPacket.data).c_str());
            //     ArduinoJson::JsonArray path = rrepDoc["path"].to<ArduinoJson::JsonArray>();
            //     // add last hop to the routing table
            //     auto lastHop = path[path.size() - 1].as<std::string>();
            //     this->insertIntoRoutingTable(lastHop, 0, millis(), this->duckRadio.getSNR(), this->duckRadio.getRSSI());
            //     loginfo_ln("Last hop: %s", lastHop.c_str());
            //   return;
            // } else {

            //   //update route response path by deleting last node in path
            //     path.remove(path.end());
            //     rrepDoc["path"] = path;
            //     // serialize the updated RREP packet
            //     std::string strRREP;
            //     serializeJson(rrepDoc, strRREP);
            //     auto destinationDUID = path.end()->as<std::string>();
            //     // this->txPacket->prepareForSending(&this->filter, duckutils::stringToArray<uint8_t,8>(destinationDUID),
            //     //                             DuckType::LINK, reservedTopic::rrep,
            //     //                             duckutils::stringToByteVector(strRREP));
            //     // err = this->duckRadio.sendData(this->txPacket->asBytes());
            //     // if (err != DUCK_ERR_NONE) {
            //     //     logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
            //     // } else {
                     loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
            //     // }
            // }
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
          err = this->relayPacket(rxPacket);
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
