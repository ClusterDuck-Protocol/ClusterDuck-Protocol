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
    void handleReceivedPacket() override {
        loginfo_ln("====> handleReceivedPacket: START");

        int err;
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData) {
        logerr_ln("ERROR failed to get data from DuckRadio.");
        return;
        }
        CdpPacket rxPacket(rxData.value());
        logdbg_ln("Got data from radio. size: %d",rxPacket.size());

        // recvDataCallback(rxPacket); crashes the duck if callback body not defined in sketch
        
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

    void ifBroadcast(CdpPacket rxPacket, int err) {
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
                RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                rrepDoc.addToPath(this->duid);
                this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                // Update routing table with signal info
                this->router.insertIntoRoutingTable(rxPacket.sduid, rxPacket.sduid, this->getSignalScore()); //can only be one hop away
                break;
            }
            case reservedTopic::ping:
                loginfo_ln("PING received. Sending PONG!");
                err = this->sendPong();
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("ERROR failed to send pong message. rc = %d",err);
                }
                break;
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
                RouteJSON rreqDoc = RouteJSON(rxPacket.data);
                //route requests are just forwarded so we can use the sduid as the origin
                std::optional<Duid> last = rreqDoc.getlastInPath();
                Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                //addToPath
                if(!relay) {
                    loginfo_ln("handleReceivedPacket: Sending RREP");
                    rreqDoc.convertReqToRep();
                    rxPacket.data = duckutils::stringToByteVector(rreqDoc.addToPath(this->duid));
                    this->sendRouteResponse(lastInPath, rreqDoc.asString());
                } else {
                    rxPacket.data = duckutils::stringToByteVector(rreqDoc.addToPath(this->duid)); //why is this different from stringToArray
                    err = this->forwardPacket(rxPacket);
                    if (err != DUCK_ERR_NONE) {
                        logerr_ln("====> ERROR handleReceivedPacket failed to relay RREQ. rc = %d",err);
                    } else {
                        loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                    }
                }
                //if last is null send self
                this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
            }
            break;
          
            case reservedTopic::rrep: {
                //we still need to recieve rreps in case of ttl expiry
                RouteJSON rrepDoc = RouteJSON(rxPacket.data);
                std::optional<Duid> last = rrepDoc.getlastInPath();
                Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                if(rrepDoc.getDestination() != this->duid){
                    loginfo_ln("Received Route Response from DUID: %s", rxPacket.sduid.data(), rxPacket.sduid.size());

                    rrepDoc.removeFromPath(this->duid);
                    //route responses need a way to keep tray of who relayed the packet, but a response needs to be directed and not broadly relayed
                    this->sendRouteResponse(lastInPath, rrepDoc.asString()); //so here the relaying duck is known from sduid
                }
                Serial.println(" +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ rrep already seen");
                //destination = sender of the rrep -> the last hop to current duck
                this->router.insertIntoRoutingTable(rrepDoc.getOrigin(), lastInPath, this->getSignalScore());
            }
                break;
            case reservedTopic::ping:
                loginfo_ln("PING received. Sending PONG!");
                err = this->sendPong();
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("ERROR failed to send pong message. rc = %d",err);
                }
                break;
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
                if(relay){
                    Serial.println(" ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ default behavior");
                    this->forwardPacket(rxPacket); 
                }       
        }
    }

};

#endif