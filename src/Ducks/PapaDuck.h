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
   * The callback is invoked for every packet the papa hears, including repeated
   * copies of the same message relayed by different ducks. Duplicates are meant
   * to be removed downstream (e.g. in the cloud), where every copy is still useful
   * for coverage. Use muidNotReceived() to drop them locally instead.
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

  /// True if no copy of this message has been handled yet. Optional local dedup.
  bool muidNotReceived(Muid muid){
   return this->router.getFilter().bloom_check(muid.data(), MUID_LENGTH) == 0;
  }

  /**
   * @brief Send a UTC time to the mesh as topics::cmd_time, {"epoch":<seconds>}.
   *
   * Ducks built with CDPCFG_REMOTE_TIME_SYNC set their clock from it; other
   * ducks only relay it. Send only a time you trust, such as one from NTP or
   * the cellular network, and resend it periodically: a duck that missed it,
   * or rebooted after a power loss, has no time until the next one.
   * @param epoch UTC seconds since 1970, at least CDPCFG_TIME_MIN_EPOCH
   * @param target BROADCAST_DUID for every duck, or one duck's DUID
   * @returns DUCK_ERR_NONE if queued, DUCK_ERR_INVALID_ARGUMENT for an epoch
   * below CDPCFG_TIME_MIN_EPOCH, or the sendData() error.
   */
  int sendTimeSync(uint32_t epoch, Duid target = BROADCAST_DUID) {
    if (epoch < CDPCFG_TIME_MIN_EPOCH) {
      logerr_ln("ERROR sendTimeSync epoch %lu is before %lu", (unsigned long)epoch,
        (unsigned long)CDPCFG_TIME_MIN_EPOCH);
      return DUCK_ERR_INVALID_ARGUMENT;
    }
    JsonDocument doc;
    doc["epoch"] = epoch;
    std::string payload;
    serializeJson(doc, payload);
    return this->sendData(topics::cmd_time, payload, target);
  }

protected:
  /**
   * @brief Hand the PapaDuck's own health message to the receive callback.
   *
   * The PapaDuck is where health messages go, so sending its own over LoRa
   * would only use airtime. The callback gets it as a packet from this duck to
   * PAPADUCK_DUID with hop count 0, the same way it gets health from the mesh,
   * so the application can publish it to the cloud.
   * @returns DUCK_ERR_NONE if delivered, DUCK_ERR_NOT_SUPPORTED without a
   * callback, DUCKPACKET_ERR_SIZE_INVALID for an empty or oversized message.
   */
  int reportHealth(const std::string& message) override {
    if (!recvDataCallback) return DUCK_ERR_NOT_SUPPORTED;
    CdpPacket packet(PAPADUCK_DUID, topics::health,
      std::vector<uint8_t>(message.begin(), message.end()), this->duid, getType());
    this->router.getFilter().assignUniqueMessageId(packet);
    const int err = packet.prepareForSending();
    if (err != DUCK_ERR_NONE) return err;
    CdpPacket delivered(packet.asBytes());
    delivered.timeReceived = millis();
    recvDataCallback(delivered);
    return DUCK_ERR_NONE;
  }

private:
  rxDoneCallback recvDataCallback = nullptr;
  
  void handleReceivedPacket(CdpPacket rxPacket) {
    logdbg_ln("====> handleReceivedPacket: START");
    
    if (recvDataCallback) recvDataCallback(rxPacket);

    //Check if Duck is desitination for this packet before relaying
    if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
        ifBroadcast(rxPacket);
    } else if(duckutils::isEqual(this->duid, rxPacket.dduid) || duckutils::isEqual(rxPacket.dduid, PAPADUCK_DUID)) { //Target device check
        ifNotBroadcast(rxPacket);
    } else { //If it's meant for a specific target but not this one
        ifNotBroadcast(rxPacket, true);
    }
    this->router.getFilter().bloom_add(rxPacket.muid.data(), MUID_LENGTH);
  } 

  void ifBroadcast(CdpPacket rxPacket) { 
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
        // case reservedTopic::cmd:
        //     loginfo_ln("Command received");

        //     err = this->broadcastPacket(rxPacket);
            
        //     if (err != DUCK_ERR_NONE) {
        //         logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
        //     } else {
        //         loginfo_ln("handleReceivedPacket: packet RELAY DONE");
        //     }
        //     break;
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
        // case reservedTopic::cmd:
        //     loginfo_ln("Command received");

        //     err = this->broadcastPacket(rxPacket);
            
        //     if (err != DUCK_ERR_NONE) {
        //         logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
        //     } else {
        //         loginfo_ln("handleReceivedPacket: packet RELAY DONE");
        //     }
        //     break;
        default:
          if(relay){
            this->forwardPacket(rxPacket);
          }               
    }
  }

};

#endif
