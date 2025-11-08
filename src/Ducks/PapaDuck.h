#ifndef PAPADUCK_H
#define PAPADUCK_H

#include "Duck.h"
#include "../wifi/DuckWifi.h"

template <typename WifiCapability = DuckWifi, typename RadioType = DuckLoRa>
class PapaDuck : public Duck<WifiCapability, RadioType> {
public:
  using Duck<WifiCapability, RadioType>::Duck;
  
  PapaDuck(std::string name = "PAPADUCK") : Duck<WifiCapability, RadioType>(std::move(name)) {}
  ~PapaDuck() {}

  /// Papa Duck callback functions signature.
  using rxDoneCallback = void (*)(CdpPacket data);
  using txDoneCallback = void (*)(void);
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  DuckType getType() { return DuckType::PAPA; }

  //remove this when mqtt quack pack is added
  bool isWifiConnected(){
    return this->duckWifi.connected();
  }

private:
  rxDoneCallback recvDataCallback;
  
  void handleReceivedPacket() {
    int err;
    std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
    if (!rxData) {
    logerr_ln("ERROR failed to get data from DuckRadio.");
    return;
    }
    CdpPacket rxPacket(rxData.value());
    logdbg_ln("Got data from radio. size: %d",rxPacket.size());

    recvDataCallback(rxPacket);
    loginfo_ln("handleReceivedPacket: START");

    //Check if Duck is desitination for this packet before relaying
    if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
        ifBroadcast(rxPacket, err);
    } else if(duckutils::isEqual(this->duid, rxPacket.dduid) || duckutils::isEqual(rxPacket.dduid, PAPADUCK_DUID)) { //Target device check
        ifNotBroadcast(rxPacket, err);
    } else { //If it's meant for a specific target but not this one
        ifNotBroadcast(rxPacket, err, true);
    }
  } 

  void ifBroadcast(CdpPacket rxPacket, int err) {
    switch(rxPacket.topic) {
        case reservedTopic::rreq: {
            loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
            RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
            this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
            // Update routing table with signal info
            this->router.insertIntoRoutingTable(rxPacket.sduid, rrepDoc.getlastInPath(), this->getSignalScore());
            break;
        }
        case reservedTopic::ping:
            loginfo_ln("PING received. Sending PONG!");
            err = this->sendPong();
            if (err != DUCK_ERR_NONE) {
                logerr_ln("ERROR failed to send pong message. rc = %d",err);
            }
            return;
        case reservedTopic::pong:
            loginfo_ln("PONG received. Ignoring!");
            break;
        case reservedTopic::cmd:
            loginfo_ln("Command received");

            err = this->broadcastPacket(rxPacket);
            
            if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
            } else {
                loginfo_ln("handleReceivedPacket: packet RELAY DONE");
            }
            break;
        default:
            err = this->broadcastPacket(rxPacket);
            if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
            } else {
                loginfo_ln("handleReceivedPacket: packet RELAY DONE");
            }
    }
}

void ifNotBroadcast(CdpPacket rxPacket, int err, bool relay = false) {
    switch(rxPacket.topic) {
        case reservedTopic::rreq: {
            RouteJSON rreqDoc = RouteJSON(rxPacket.asBytes());
            if(!relay) {
                loginfo_ln("handleReceivedPacket: Sending RREP");
                this->sendRouteResponse(rreqDoc.getlastInPath(), rreqDoc.asString());
                return;//should this be here?
            } else {
                loginfo_ln("RREQ received for relay. Relaying!");
                rxPacket.data = duckutils::stringToByteVector(rreqDoc.addToPath(this->duid)); //why is this different from stringToArray
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay RREQ. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                }
            }
            this->router.insertIntoRoutingTable(rxPacket.sduid, rreqDoc.getlastInPath(), this->getSignalScore());
        }
        break;
      
        case reservedTopic::rrep: {
            //we still need to recieve rreps in case of ttl expiry
            RouteJSON rrepDoc = RouteJSON(rxPacket.asBytes());
            if(relay){ 
                loginfo_ln("Received Route Response from DUID: %s", duckutils::convertToHex(rxPacket.sduid.data(), rxPacket.sduid.size()).c_str());

                rrepDoc.removeFromPath(this->duid);
                //route responses need a way to keep tray of who relayed the packet, but a response needs to be directed and not broadly relayed
                this->sendRouteResponse(rrepDoc.getlastInPath(), rrepDoc.asString()); //so here the relaying duck is known from sduid
            }
            //destination = sender of the rrep -> the last hop to current duck
            Duid thisId = rxPacket.sduid;
            this->router.insertIntoRoutingTable(rrepDoc.getDestination(), thisId, this->getSignalScore()); //why do i need to copy here 
        }
            break;
        default:
          if(relay){
            this->forwardPacket(rxPacket);
          }               
    }
  }

};

#endif
