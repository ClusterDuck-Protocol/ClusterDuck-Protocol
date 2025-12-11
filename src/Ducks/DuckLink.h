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
    /**
     * @brief Handles any packets received by the duck. Overrides the pure virtual function in Duck base class.
     */
    void handleReceivedPacket() override{
      if (this->duckRadio.getReceiveFlag()){
          bool relay = false;
          
          loginfo_ln("====> handleReceivedPacket: START");
  
          int err;
          std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
          if (!rxData) {
          logerr_ln("ERROR failed to get data from DuckRadio.");
          return;
          }
          CdpPacket rxPacket(rxData.value());
          logdbg_ln("Got data from radio. size: %d",rxPacket.size());
  
          // recvDataCallback(rxPacket.asBytes());
          
          //Check if Duck is desitination for this packet before relaying
          if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
              ifBroadcast(rxPacket, err);
          } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
              ifNotBroadcast(rxPacket, err);
          } else { //If it's meant for a specific target but not this one
              ifNotBroadcast(rxPacket, err, true);
          }
          this->router.getFilter().bloom_add(rxPacket.muid.data(), MUID_LENGTH);
      }
    }
  
      void ifBroadcast(CdpPacket rxPacket, int err) {
          switch(rxPacket.topic) {
              case reservedTopic::rreq: {
                  loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
                  Serial.print("hops on rreq:       ");
                  Serial.println(rxPacket.hopCount);
                  RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                  this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                  // Update routing table with signal info
                  std::optional<Duid> last = rrepDoc.getlastInPath();
                  Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                  this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
                  break;
              }
              case reservedTopic::ping:
                  loginfo_ln("PING received. Sending PONG!");
                  err = this->sendPong();
                  if (err != DUCK_ERR_NONE) {
                      logerr_ln("ERROR failed to send pong message. rc = %d",err);
                  }
                  break;
              case reservedTopic::cmd:
                  loginfo_ln("Command received");
                  err = this->broadcastPacket(rxPacket);
                  break;
              default:
                  loginfo_ln("handleReceivedPacket: packet received, skipping relay.");
          }
      }
  
      void ifNotBroadcast(CdpPacket rxPacket, int err, bool relay = false) {
          switch(rxPacket.topic) {
              case reservedTopic::rreq: {
                  RouteJSON rreqDoc = RouteJSON(rxPacket.data);
                  if(!relay) {
                      loginfo_ln("handleReceivedPacket: Sending RREP");
                      std::optional<Duid> last = rreqDoc.getlastInPath();
                      Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                      this->sendRouteResponse(lastInPath, rreqDoc.asString());
                      this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
                  }
                  Serial.println(" ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ this is really weird");
              }
                break;
              case reservedTopic::rrep: {
                  //we still need to recieve rreps in case of ttl expiry
                  RouteJSON rrepDoc = RouteJSON(rxPacket.data);
                  loginfo_ln("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Received Route Response from DUID: %s", rxPacket.sduid.data());
                  //destination = sender of the rrep -> the last hop to current duck
                  std::optional<Duid> last = rrepDoc.getlastInPath();
                  Duid lastInPath = last.value();
                  
                  rrepDoc.convertReqToRep();
                  this->router.insertIntoRoutingTable(rrepDoc.getOrigin(), lastInPath, this->getSignalScore());
              }
                  break;
              default:
              loginfo_ln("handleReceivedPacket: packet received, skipping forward.");    
          }
      }
};

#endif
