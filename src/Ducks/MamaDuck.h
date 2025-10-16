#ifndef MAMADUCK_H
#define MAMADUCK_H

#include "Duck.h"
#include "../utils/MemoryFree.h"

template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class MamaDuck : public Duck<WifiCapability, RadioType> {
public:
    using Duck<WifiCapability, RadioType>::Duck;

    MamaDuck(std::string name = "MAMA0001") : Duck<WifiCapability, RadioType>(std::move(name)) {}

    ~MamaDuck() {};

    using rxDoneCallback = void (*)(CdpPacket data);
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
    DuckType getType() {return DuckType::MAMA;}

private :
    WifiCapability duckWifi;
    rxDoneCallback recvDataCallback;
    /**
     * @brief Handles any packets received by the duck. Overrides the pure virtual function in Duck base class.
     * Could be a RREQ, RREP, PING, PONG or DATA packet on its associated topic.
     *
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
        logdbg_ln("Got data from radio, prepare for relay. size: %d",rxPacket.size());

        // recvDataCallback(rxPacket.asBytes());
        loginfo_ln("handleReceivedPacket: packet RELAY START");
        
        //Check if Duck is desitination for this packet before relaying
        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
            ifBroadcast(rxPacket, err);
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            ifNotBroadcast(rxPacket, err);
        } else { //If it's meant for a specific target but not this one
            ifNotBroadcast(rxPacket, err, true);
        }
    }
    }

    void ifBroadcast(CdpPacket rxPacket, int err) {
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
                RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                // Update routing table with signal info
                this->router.insertIntoRoutingTable(rxPacket.sduid, this->getSignalScore(), millis());
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

                err = this->relayPacket(rxPacket);
                
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
                break;
            default:
                err = this->relayPacket(rxPacket);
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
                    return;
                } else {
                    loginfo_ln("RREQ received for relay. Relaying!");
                    rxPacket.data = duckutils::stringToByteVector(rreqDoc.addToPath(this->duid)); //why is this different from stringToArray
                    err = this->relayPacket(rxPacket);
                    if (err != DUCK_ERR_NONE) {
                        logerr_ln("====> ERROR handleReceivedPacket failed to relay RREQ. rc = %d",err);
                    } else {
                        loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                    }
                }
            }
            break;
          
            case reservedTopic::rrep: {
                // if(relay){ //might not need this because a duck that isn't in network can't use handleReceivedPacket
                    loginfo_ln("Received Route Response from DUID: %s", duckutils::convertToHex(rxPacket.sduid.data(), rxPacket.sduid.size()).c_str());
                    RouteJSON rrepDoc = RouteJSON(rxPacket.asBytes());
                    rrepDoc.removeFromPath(this->duid);
                    this->sendRouteResponse(rrepDoc.getlastInPath(), rrepDoc.asString); //so here the relaying duck is known from sduid
                // }
            }
                break;
            default:
                //check where to relay it to 
                //use packet.dduid as key to retrieve best next hop
                //forward packet only to next hop
                err = this->relayPacket(rxPacket); //make a relayBroadcast and relayDirect?
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
        }
    }

};

#endif