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

    using rxDoneCallback = void (*)(std::vector<byte> data );
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
        // NOTE:
        // Ducks will only handle received message one at a time, so there is a chance the
        // packet being sent below will never be received, especially if the cluster is small
        // there are not many alternative paths to reach other mama ducks that could relay the rxPacket.
        
        //Check if Duck is desitination for this packet before relaying
        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
            ifBroadcast(rxPacket, err);
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            ifNotBroadcast(rxPacket, err);
        } else {
            err = this->relayPacket(rxPacket);
            if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
            } else {
            loginfo_ln("handleReceivedPacket: packet RELAY DONE");
            }
        }
    }
    }

    void ifBroadcast(CdpPacket rxPacket, int err) {
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                loginfo_ln("RREQ received from %s. Updating RREQ!", rxPacket.sduid.data());
                RouteJSON rreqDoc = RouteJSON(rxPacket.asBytes());
                std::string rreq = rreqDoc.addToPath(this->deviceId);
                this->sendRouteResponse(rxPacket.sduid, rreq);
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

    void ifNotBroadcast(CdpPacket rxPacket, int err) {
        std::vector<uint8_t> dataPayload;
        uint8_t num = 1;

        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                loginfo_ln("RREQ received. Updating RREQ!");
                RouteJSON rreqDoc = RouteJSON(rxPacket.asBytes());
                loginfo_ln("handleReceivedPacket: Sending RREP");
                //add current duck to path
                //update the rreq to make it into a rrep
                //Serialize the updated RREQ packet
                std::string strRREP = rreqDoc.addToPath(this->deviceId);
                this->sendRouteResponse(PAPADUCK_DUID, dataPayload); //was this meant to be prepareforsending an rxPacket instead txPacket?
                return;
            }
            break;
            case reservedTopic::rrep: {
                loginfo_ln("Received Route Response from DUID: %s", duckutils::convertToHex(rxPacket.sduid.data(), rxPacket.sduid.size()).c_str());
                //pop current duck off of path and send to the next duck to get to origin
                
                //if duck is not in a network already
                this->setNetworkState(NetworkState::PUBLIC);
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