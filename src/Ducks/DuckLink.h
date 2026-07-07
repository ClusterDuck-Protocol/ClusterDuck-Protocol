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
     * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
     * @param cb a callback to handle data received by the papa duck
     */
    void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  private:
    rxDoneCallback recvDataCallback;

    /**
     * @brief Handles any packets received by the duck. Overrides the pure virtual function in Duck base class.
     */
    void handleReceivedPacket(CdpPacket rxPacket) override{
        bool relay = false;
        
        loginfo_ln("====> handleReceivedPacket: START");

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
              case reservedTopic::rreq: {
                if(rxPacket.hopCount <= 0){
                  loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
                  RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                  this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                  // Update routing table with signal info
                  std::optional<Duid> last = rrepDoc.getlastInPath();
                  Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                  if(rxPacket.duckType == DuckType::PAPA){
                    this->router.insertIntoRoutingTable(PAPADUCK_DUID, lastInPath, this->getSignalScore());
                  } else {
                    this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
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
              case topics::cmd_bat:{
                  ArduinoJson::JsonDocument json;
                  std::string packetStr(rxPacket.data.begin(), rxPacket.data.end());
                  DeserializationError error = deserializeJson(json, packetStr);
                  if (error) {
                      logerr_ln("Duck Command cmd_tx deserialization failed: %s", error.c_str());
                      break;
                  }
                  float battery_min = json["min"];
                  float battery_max = json["max"];
                  loginfo_ln("Command received, updating battery sleep thresholds: off(min)=%.3fV on(max)=%.3fV", battery_min, battery_max);
                  if ((battery_min < BAT_V_EMPTY || battery_min > BAT_V_FULL) || (battery_max < BAT_V_EMPTY || battery_max > BAT_V_FULL)) {
                    logerr_ln("Invalid argument -- battery threshold min: %f , battery threshold max %f", battery_min, battery_max);
                    err = DUCK_ERR_INVALID_ARGUMENT;
                    break;
                  }

                  if (err == DUCK_ERR_NONE){
                    // min = Teensy power-off threshold, max = power-on/recovery
                    // threshold, both in volts. Store as float under the same
                    // keys seeded in Duck::setupWithDefaults.
                    const size_t offBytes = this->eeprom_preferences.putFloat("teensy_off", battery_min);
                    const size_t onBytes = this->eeprom_preferences.putFloat("teensy_on", battery_max);
                    if (offBytes != sizeof(float) || onBytes != sizeof(float)) {
                      logerr_ln("Failed to persist battery sleep thresholds to Preferences");
                      err = DUCK_ERR_EEPROM_WRITE;
                      break;
                    }
                  }

                  err = this->broadcastPacket(rxPacket);
                  if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                    } else {
                        loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                    }
                  break;
              }
              case topics::cmd_tx:{
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

                  if ((tx_pwr < 0 || tx_pwr > 100) || (sf < 7 || sf > 12)) {
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
                        logerr("JSON parse failed: ");
                        logerr(error.c_str());
                    } else{
                      uint32_t epoch = json["epoch"].as<uint32_t>();
                      this->rtc.setTime(epoch);
                      Serial.printf("RTC set to %u\n", epoch);
                    }
                  break;
              }
              default:
                  loginfo_ln("handleReceivedPacket: packet received, skipping relay.");
          }
      }
  
      void ifNotBroadcast(CdpPacket rxPacket, bool relay = false) {
        int err = DUCK_ERR_NONE;
          switch(rxPacket.topic) {
              case reservedTopic::rreq: {
                  RouteJSON rreqDoc = RouteJSON(rxPacket.data);
                  if (!rreqDoc.isValid()) {
                      logerr_ln("handleReceivedPacket: dropping malformed RREQ");
                      break;
                  }
                  if(!relay) {
                      loginfo_ln("handleReceivedPacket: Sending RREP");
                      std::optional<Duid> last = rreqDoc.getlastInPath();
                      Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                      rreqDoc.convertReqToRep();
                      this->sendRouteResponse(lastInPath, rreqDoc.asString());
                      if(rxPacket.duckType == DuckType::PAPA){
                        this->router.insertIntoRoutingTable(PAPADUCK_DUID, lastInPath, this->getSignalScore());
                      } else {
                        this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
                      }
                  }
              }
                break;
              case reservedTopic::rrep: {
                  //we still need to recieve rreps in case of ttl expiry
                  RouteJSON rrepDoc = RouteJSON(rxPacket.data);
                  if (!rrepDoc.isValid()) {
                      logerr_ln("handleReceivedPacket: dropping malformed RREP");
                      break;
                  }
                  std::string sourceDuid(rxPacket.sduid.begin(), rxPacket.sduid.end());
                  loginfo_ln("Received Route Response from DUID: %s", sourceDuid.c_str());
                  //destination = sender of the rrep -> the last hop to current duck
                  std::optional<Duid> last = rrepDoc.getlastInPath();
                  Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;

                  this->router.insertIntoRoutingTable(rrepDoc.getOrigin(), lastInPath, this->getSignalScore());
              }
                  break;
              default:
              loginfo_ln("handleReceivedPacket: packet received, skipping forward.");    
          }
      }
};

#endif
