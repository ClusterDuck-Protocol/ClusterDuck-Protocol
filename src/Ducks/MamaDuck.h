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
     * The callback is invoked for every packet the duck hears, including repeated
     * copies of the same message relayed by different ducks.
     * @param cb a callback to handle data received by the mama duck
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
    rxDoneCallback recvDataCallback = nullptr;
    /**
     * @brief Handles any packets received by the duck. Overrides the pure virtual function in Duck base class.
     * Could be a RREQ, RREP, PING, PONG or DATA packet on its associated topic.
     *
     */
    void handleReceivedPacket(CdpPacket rxPacket) override {
        logdbg_ln("====> handleReceivedPacket: START");
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

    /**
     * @brief Handle a packet addressed to every duck (BROADCAST_DUID).
     *
     * Route requests and replies never reach this function:
     * Duck::processReceivedPacket() hands them to handleRoutingPacket() first.
     * - PING: answered with a broadcast PONG. Neither is relayed, so PING and
     *   PONG only reach direct neighbours.
     * - PONG: ignored.
     * - cmd_tx, if CDPCFG_REMOTE_TX_POWER is on: stores the new TX power
     *   (14 to 22 dBm) for the next boot, if this duck's radio takes it, and
     *   relays the packet. A command that doesn't parse or is outside 14 to 22
     *   dBm is neither applied nor relayed.
     * - cmd_time, if CDPCFG_REMOTE_TIME_SYNC is on: applyTimeCommand(), then
     *   relayed whether or not the time was applied.
     * - Anything else, including cmd_tx and cmd_time when their setting is
     *   off: relayed with broadcastPacket(), which drops packets it has
     *   already seen and packets that reached MAX_HOPS.
     *
     * @param rxPacket the received packet
     */
    void ifBroadcast(CdpPacket rxPacket) {
        int err = DUCK_ERR_NONE;
        switch(rxPacket.topic) {
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
#if CDPCFG_REMOTE_TX_POWER
            case topics::cmd_tx: {
                ArduinoJson::JsonDocument json;
                  std::string packetStr(rxPacket.data.begin(), rxPacket.data.end());
                  DeserializationError error = deserializeJson(json, packetStr);
                  if (error) {
                      logerr_ln("Duck Command cmd_tx deserialization failed: %s", error.c_str());
                      break;
                  }
                  loginfo_ln("Command received, updating transmission power");
                  
                  int txPwr = json["txPwr"];

                  if (txPwr < 14 || txPwr > 22) {
                    logerr_ln("Invalid argument -- tx power: %i", txPwr);
                    err = DUCK_ERR_INVALID_ARGUMENT;
                    break;
                  }

                  // Store only a value this duck's radio takes, or radio setup
                  // fails at the next boot. Relay it either way: other ducks
                  // may have a different radio.
                  if (this->duckRadio.checkTxPower(txPwr) != DUCK_ERR_NONE) {
                    logerr_ln("tx power %i dBm not supported by this radio, not stored", txPwr);
                  } else {
                    Serial.print("updating tx power to value : ");
                    Serial.println(txPwr);
                    this->eeprom_preferences.putInt("txPwr", txPwr);
                  }
                 
                  err = this->broadcastPacket(rxPacket);
                  if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                    } else {
                        loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                    }
                  break;
            }
#endif
#if CDPCFG_REMOTE_TIME_SYNC
            case topics::cmd_time:
                // Must run before the relay below, which marks the message as seen.
                // Relayed whether or not it was applied, like any other packet.
                this->applyTimeCommand(rxPacket);
                [[fallthrough]];
#endif
            default:
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
        }
    }

    /**
     * @brief Handle a packet addressed to a single duck.
     *
     * Route requests and replies never reach this function:
     * Duck::processReceivedPacket() hands them to handleRoutingPacket() first.
     * - PING: answered with a broadcast PONG, even when relay is true. Not
     *   passed on.
     * - PONG: ignored, not passed on.
     * - cmd_tx and cmd_time, if their setting is on: applied when addressed to
     *   this duck, and then not passed on. cmd_tx is stored only if it is 14 to
     *   22 dBm and this duck's radio takes it. Passed on with forwardPacket()
     *   when addressed to another duck.
     * - Anything else: passed on with forwardPacket() when relay is true. A
     *   packet addressed to this duck only goes to the onReceiveDuckData()
     *   callback, which handleReceivedPacket() has already called.
     *
     * forwardPacket() floods like broadcastPacket(): it doesn't need a route
     * to the destination.
     *
     * @param rxPacket the received packet
     * @param relay true if the packet is addressed to another duck
     */
    void ifNotBroadcast(CdpPacket rxPacket, bool relay = false) {
        int err;
        switch(rxPacket.topic) {
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
#if CDPCFG_REMOTE_TX_POWER
            case topics::cmd_tx: {
                // A command for another duck is passed on, not applied here.
                if (relay) {
                    this->forwardPacket(rxPacket);
                    break;
                }
                // Targeted cmd_tx used to log and do nothing, so a command aimed at a
                // single duck was silently dropped. Apply the same update as the
                // broadcast case; deliberately no relay, since this one is addressed
                // to us.
                ArduinoJson::JsonDocument json;
                std::string packetStr(rxPacket.data.begin(), rxPacket.data.end());
                DeserializationError error = deserializeJson(json, packetStr);
                if (error) {
                    logerr_ln("Duck Command cmd_tx deserialization failed: %s", error.c_str());
                    break;
                }
                loginfo_ln("Command received, updating transmission power");

                int txPwr = json["txPwr"];
                if (txPwr < 14 || txPwr > 22) {
                    logerr_ln("Invalid argument -- tx power: %i", txPwr);
                    err = DUCK_ERR_INVALID_ARGUMENT;
                    break;
                }
                // Store only a value this duck's radio takes, or radio setup
                // fails at the next boot.
                if (this->duckRadio.checkTxPower(txPwr) != DUCK_ERR_NONE) {
                    logerr_ln("tx power %i dBm not supported by this radio, not stored", txPwr);
                    break;
                }
                this->eeprom_preferences.putInt("txPwr", txPwr);
                loginfo_ln("updating tx power to value : %i", txPwr);
                break;
            }
#endif
#if CDPCFG_REMOTE_TIME_SYNC
            case topics::cmd_time:
                // Addressed to us: apply it, no relay. Otherwise pass it on.
                if (relay) this->forwardPacket(rxPacket);
                else this->applyTimeCommand(rxPacket);
                break;
#endif
            default:
                if(relay){
                    this->forwardPacket(rxPacket); 
                }       
        }
    }

};

#endif
