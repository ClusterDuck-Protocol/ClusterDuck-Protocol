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

    using rxDoneCallback = void (*)(CdpPacket data);
    /**
     * @brief Register callback for handling data received from duck devices
     * 
     * The callback is invoked for every packet the duck hears, including repeated
     * copies of the same message relayed by different ducks.
     * @param cb a callback to handle data received by the duck link
     */
    void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  private:
    rxDoneCallback recvDataCallback = nullptr;

    /**
     * @brief Handles any packets received by the duck. Overrides the pure virtual function in Duck base class.
     */
    void handleReceivedPacket(CdpPacket rxPacket) override{
        bool relay = false;
        
        logdbg_ln("====> handleReceivedPacket: START");

        if (recvDataCallback) recvDataCallback(rxPacket);
        
        //Check if Duck is desitination for this packet before relaying
        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
            ifBroadcast(rxPacket);
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            ifNotBroadcast(rxPacket, false);
        } else { //If it's meant for a specific target but not this one
            ifNotBroadcast(rxPacket, true);
        }
        this->router.getFilter().bloom_add(rxPacket.muid.data(), MUID_LENGTH);
    }
  
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
#if CDPCFG_REMOTE_TX_POWER
              case topics::cmd_tx:{
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
                    this->eeprom_preferences.putInt("txPwr", txPwr);
                    Serial.print("updating tx power to value : ");
                    Serial.println(txPwr);
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
                  // A DuckLink is a leaf: it applies the time but does not relay it.
                  this->applyTimeCommand(rxPacket);
                  break;
#endif
              default:
                  logdbg_ln("handleReceivedPacket: packet received, skipping relay.");
          }
      }
  
      void ifNotBroadcast(CdpPacket rxPacket, bool relay = false) {
        int err = DUCK_ERR_NONE;
          switch(rxPacket.topic) {
#if CDPCFG_REMOTE_TIME_SYNC
              case topics::cmd_time:
                  // Apply only a command addressed to us.
                  if (!relay) this->applyTimeCommand(rxPacket);
                  break;
#endif
              default:
              logdbg_ln("handleReceivedPacket: packet received, skipping forward.");    
          }
      }
};

#endif
