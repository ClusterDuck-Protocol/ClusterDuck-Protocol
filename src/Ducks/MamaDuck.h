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
   * @brief Override the default setup method to match MamaDuck specific
   * defaults.
   *
   * In addition to Serial component, the Radio component is also initialized.

   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
   int setupWithDefaults() {

    // Initialize the serial component with the hardware supported baudrate
    this->setupSerial(115200);
  
    int err = this->setupLoRaRadio();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err);
      return err;
    }
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err);
      return err;
    }
    return DUCK_ERR_NONE;
  }

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
      logdbg_ln("Got data from radio, prepare for relay. size: %d",rxPacket.rawBuffer().size());

      recvDataCallback(rxPacket.rawBuffer());
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
                loginfo_ln("RREQ received from %s. Updating RREQ!",
                           rxPacket.sduid.data());
                ArduinoJson::JsonDocument rreqDoc;
                deserializeJson(rreqDoc, rxPacket.data);
                // CdpPacket::UpdateRREQ(rreqDoc, this->deviceId);
                loginfo_ln("handleReceivedPacket: RREQ updated with current DUID: %s", this->duid);
                //Serialize the updated RREQ packet
                std::string strRREQ;
                serializeJson(rreqDoc, strRREQ);
                // this->rxPacket->rawBuffer() = duckutils::stringToByteVector(strRREQ);
                err = this->relayPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d", err);
                } else {
                    loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                }
                return;
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
              ArduinoJson::JsonDocument rreqDoc;
              deserializeJson(rreqDoc, rxPacket.data);
              loginfo_ln("handleReceivedPacket: Sending RREP");
              //Serialize the updated RREQ packet
              std::string strRREP;
              serializeJson(rreqDoc, strRREP);
              rxPacket.rawBuffer() = duckutils::stringToByteVector(strRREP);
              err = this->relayPacket(rxPacket);
              if (err != DUCK_ERR_NONE) {
                  logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d", err);
              } else {
                  loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
              }
              this->sendRouteRequest(PAPADUCK_DUID, dataPayload); //was this meant to be prepareforsending an rxPacket instead txPacket?
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