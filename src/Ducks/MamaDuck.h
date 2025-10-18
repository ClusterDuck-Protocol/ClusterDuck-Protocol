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
                this->router.insertIntoRoutingTable(rxPacket.sduid, rrepDoc.getlastInPath(), this->getSignalScore());
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
                this->router.insertIntoRoutingTable(rxPacket.sduid, rrepDoc.getlastInPath(), this->getSignalScore());
            }
            break;
          
            case reservedTopic::rrep: {
                //we still need to recieve rreps in case of ttl expiry
                if(relay){ 
                    loginfo_ln("Received Route Response from DUID: %s", duckutils::convertToHex(rxPacket.sduid.data(), rxPacket.sduid.size()).c_str());
                    RouteJSON rrepDoc = RouteJSON(rxPacket.asBytes());
                    rrepDoc.removeFromPath(this->duid);
                    //route responses need a way to keep tray of who relayed the packet, but a response needs to be directed and not broadly relayed
                    this->sendRouteResponse(rrepDoc.getlastInPath(), rrepDoc.asString); //so here the relaying duck is known from sduid
                }
                //destination = sender of the rrep -> the last hop to current duck
                this->router.insertIntoRoutingTable(rrepDoc.getDestination(), rxPacket.sduid, this->getSignalScore(), millis());
            }
                break;
            default:
                    //next node checks if it has the destination in it's table
                    //if in table, and ttl hasn't expired, forward the packet
                    //if in the table and ttl has expired, send a rreq and wait for response before sending?<--- do this later?
                    //if the duck can't find the destination in it's routing table then it just doesn't send
                    
                    //should this happen for all relays? no it should only happen for forwarded not broadcast
                std::optional<Duid> nextHop = this->router.getBestNextHop(packet.dduid);//neighbor record?
                if(nextHop.has_value()){
                    if(nextHop.ttl > 0){
                        err = this->relayPacket(rxPacket);
                        if (err != DUCK_ERR_NONE) {
                            logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                        } else {
                            loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                        }
                    } else{
                        logdbg_ln("no entry for this id, skipping relay");
                    }
                    
                }
                
        }
    }

};

#endif