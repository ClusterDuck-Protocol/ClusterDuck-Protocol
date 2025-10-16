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
        // Update routing table with signal info from last received packet
        this->router.insertIntoRoutingTable(duckutils::toString(rxPacket.sduid), this->getSignalScore(), millis());
        logdbg_ln("Got data from radio, prepare for relay. size: %d",rxPacket.size());

        // recvDataCallback(rxPacket.asBytes());
        loginfo_ln("handleReceivedPacket: packet RELAY START");
        // NOTE:
        // Ducks will only handle received message one at a time, so there is a chance the
        // packet being sent below will never be received, especially if the cluster is small
        // there are not many alternative paths to reach other mama ducks that could relay the rxPacket.
        
        //Check if Duck is desitination for this packet before relaying
        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
            ifBroadcast(rxPacket, err);
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            ifNotBroadcast(rxPacket, err);
          /*
           * There needs to be a case for handling packets not addressed to this duck
           * but also not broadcast. This method may look very different once finished
           */
        } else { //Source device check
            ifNotBroadcast(rxPacket, err, true);
        }
    }
    }

    void ifBroadcast(CdpPacket rxPacket, int err) {
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                loginfo_ln("RREQ received from %s. Updating RREQ!", rxPacket.sduid.data());
                RouteJSON rreqDoc = RouteJSON(rxPacket.asBytes());
                std::string rreq = rreqDoc.addToPath(this->duid);
                this->sendRouteResponse(rxPacket.sduid, duckutils::stringToByteVector(rreq));
                //update routing table with sduid
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
                    loginfo_ln("RREQ received. Updating RREQ!");

                    loginfo_ln("handleReceivedPacket: Sending RREP");
                    //add current duck to path
                    //update the rreq to make it into a rrep
                    //Serialize the updated RREQ packet
                    std::string strRREP = rreqDoc.addToPath(this->duid);
                    this->sendRouteResponse(PAPADUCK_DUID,
                                            strRREP.data()); //was this meant to be prepareforsending an rxPacket instead txPacket?
                    return;
                } else {
                    loginfo_ln("RREQ received for relay. Relaying!");
                    std::string packet = rreqDoc.addToPath(this->duid);
                    rxPacket.data = duckutils::stringToByteVector(packet);
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
                loginfo_ln("Received Route Response from DUID: %s", duckutils::convertToHex(rxPacket.sduid.data(), rxPacket.sduid.size()).c_str());
                //extract path from rrep and update routing table
                RouteJSON rrepDoc = RouteJSON(rxPacket.asBytes());
                //if duck is not in a network already
                this->setNetworkState(NetworkState::PUBLIC);
                //send rrep to next hop in path
                std::string rrep = rrepDoc.removeFromPath(this->duid);
                this->sendRouteResponse(duckutils::stringToArray<uint8_t,8>(rrepDoc.getlastInPath()),duckutils::stringToByteVector(rrep));
                return;
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

};

#endif