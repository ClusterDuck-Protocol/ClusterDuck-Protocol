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
    void handleReceivedPacket(CdpPacket rxPacket) override {
        loginfo_ln("====> handleReceivedPacket: START");
        if (recvDataCallback) recvDataCallback(rxPacket);

        //Check if Duck is desitination for this packet before relaying
        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
            ifBroadcast(rxPacket);
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            ifNotBroadcast(rxPacket);
        } else { //If it's meant for a specific target but not this one
            ifNotBroadcast(rxPacket, true);
        }
        this->router.getFilter().bloom_add(rxPacket.muid.data(), MUID_LENGTH);
    }

    void ifBroadcast(CdpPacket rxPacket) {
        int err;
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                if(rxPacket.hopCount <= 0){
                    loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
                    RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                    rrepDoc.addToPath(this->duid);
                    this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                    // Update routing table with signal info
                    if(rxPacket.duckType == DuckType::PAPA){
                        this->router.insertIntoRoutingTable(PAPADUCK_DUID, PAPADUCK_DUID, this->getSignalScore());
                    } else {
                        this->router.insertIntoRoutingTable(rxPacket.sduid, rxPacket.sduid, this->getSignalScore()); //can only be one hop away
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
            case reservedTopic::pong:
                loginfo_ln("PONG received. Ignoring!");
                break;
            case topics::cmd_bat: {
                ArduinoJson::JsonDocument json;
                std::string packetStr(rxPacket.data.begin(), rxPacket.data.end());
                DeserializationError error = deserializeJson(json, packetStr);
                if (error) {
                    logerr_ln("Duck Command cmd_tx deserialization failed: %s", error.c_str());
                    break;
                }
                loginfo_ln("Command received, updating battery threshold for sleep");

                float battery_min = json["min"];
                float battery_max = json["max"];
                
                //what is the min and max possible voltages
                if ((battery_min < BAT_V_EMPTY || battery_min > BAT_V_FULL) || (battery_max < BAT_V_EMPTY || battery_max > BAT_V_FULL)) {
                logerr_ln("Invalid argument -- battery threshold min: %i , battery threshold max %i", battery_min, battery_max);
                err = DUCK_ERR_INVALID_ARGUMENT;
                break;
                }

                if (err == DUCK_ERR_NONE){
                this->eeprom_preferences.putInt("bat_min", battery_min);
                this->eeprom_preferences.putInt("bat_max", battery_max);
                }

                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
                break;
            }
            case topics::cmd_tx: {
                ArduinoJson::JsonDocument json;
                std::string packetStr(rxPacket.data.begin(), rxPacket.data.end());
                DeserializationError error = deserializeJson(json, packetStr);
                if (error) {
                    logerr_ln("Duck Command cmd_tx deserialization failed: %s", error.c_str());
                    break;
                }
                loginfo_ln("Command received, updating transmission power");
                
                int tx_pwr = json["tx_pwr"];
                int sf = json["sf"];

                if ((tx_pwr < 0 || tx_pwr > 20) || (sf < 7 || sf > 12)) {
                logerr_ln("Invalid argument -- tx power: %i , spreading factor %i", tx_pwr, sf);
                err = DUCK_ERR_INVALID_ARGUMENT;
                break;
                }

                if (err == DUCK_ERR_NONE){
                this->eeprom_preferences.putInt("tx_pwr", tx_pwr);
                this->eeprom_preferences.putInt("sf", sf);
                }
                
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
                break;
            }
              case topics::cmd_time:{
                  loginfo_ln(" !!!!!!!!!!!!!!!!!!!Command received, updating time to match papa");
                  ArduinoJson::JsonDocument json;
                  std::string packetStr(rxPacket.data.begin(), rxPacket.data.end());

                    DeserializationError error = deserializeJson(json, packetStr);
                    if (error) {
                        Serial.print("JSON parse failed: ");
                        Serial.println(error.c_str());
                        return;
                    }

                    uint32_t epoch = json["epoch"].as<uint32_t>();

                    this->rtc.setTime(epoch);

                    Serial.printf("RTC set to %u\n", epoch);

                  err = this->broadcastPacket(rxPacket);
                  if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                    } else {
                        loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                    }
                  break;
              }
            default:
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
        }
    }

    void ifNotBroadcast(CdpPacket rxPacket, bool relay = false) {
        int err;
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                RouteJSON rreqDoc = RouteJSON(rxPacket.data);
                //route requests are just forwarded so we can use the sduid as the origin
                std::optional<Duid> last = rreqDoc.getlastInPath();
                Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                if(!relay) {
                    loginfo_ln("handleReceivedPacket: Sending RREP");
                    rxPacket.data = duckutils::stringToByteVector(rreqDoc.convertReqToRep());
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
            }
            break;
          
            case reservedTopic::rrep: {
                //we still need to recieve rreps in case of ttl expiry
                RouteJSON rrepDoc = RouteJSON(rxPacket.data);
                std::optional<Duid> last = rrepDoc.getlastInPath();
                Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                std::string sourceDuid(rxPacket.sduid.begin(), rxPacket.sduid.end());
                loginfo_ln("Received Route Response from DUID: %s", sourceDuid.c_str());

                std::optional<Duid> nextHop;
                if(rxPacket.duckType == DuckType::PAPA){
                    nextHop = this->router.getBestNextHop(PAPADUCK_DUID);
                } else {
                    nextHop = this->router.getBestNextHop(rrepDoc.getDestination());
                }
                
                if((rrepDoc.getDestination() != this->duid) && (nextHop.has_value()) && (nextHop.value() !=  rxPacket.sduid)){
                    rrepDoc.popFromPath();
                    rrepDoc.addToPath(this->duid);
                    //route responses need a way to keep tray of who relayed the packet, but a response needs to be directed and not broadly relayed
                    this->sendRouteResponse(rrepDoc.getDestination(), rrepDoc.asString()); //so here the "relaying" duck is known from sduid
                    if(rxPacket.duckType == DuckType::PAPA){
                        this->router.insertIntoRoutingTable(PAPADUCK_DUID, lastInPath, this->getSignalScore());
                    } else {
                        this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
                    }
                } else {
                    //destination = sender of the rrep -> the last hop to current duck
                    if(rxPacket.duckType == DuckType::PAPA){
                        this->router.insertIntoRoutingTable(PAPADUCK_DUID, lastInPath, this->getSignalScore());
                    } else {
                        this->router.insertIntoRoutingTable(rrepDoc.getOrigin(), lastInPath, this->getSignalScore());
                    }
                }
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
            case topics::cmd_tx:
                loginfo_ln("Command received, updating transmission power");
                // err = this->broadcastPacket(rxPacket);// send ack only not broadcast
                // if (err != DUCK_ERR_NONE) {
                //   logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                //   } else {
                //       loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                //   }
                break;
            default:
                if(relay){
                    this->forwardPacket(rxPacket); 
                }       
        }
    }

};

#endif
