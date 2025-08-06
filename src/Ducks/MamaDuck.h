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
   * When ssid and password are provided the duck will setup the wifi related
   * components.
   *
   * @param deviceId required device unique id
   * @param ssid wifi access point ssid (defaults to an empty string if not
   * provided)
   * @param password wifi password (defaults to an empty string if not provided)
   * 
   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
   int setupWithDefaults(std::array<byte,8> deviceId, std::string ssid = "", std::string password = "") {
  
    int err = this->setupLoRaRadio();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err);
      return err;
    }
    err = duckWifi.setupAccessPoint();
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
  int getType() {return DuckType::MAMA;}

private :
  WifiCapability duckWifi;
  rxDoneCallback recvDataCallback;

  void handleReceivedPacket() override{
    if (this->duckRadio.getReceiveFlag()){
      std::vector<uint8_t> data;
      bool relay = false;
      
      loginfo_ln("====> handleReceivedPacket: START");
  
      int err = this->duckRadio.readReceivedData(&data);
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR failed to get data from DuckRadio. rc = %d",err);
        return;
      }
      logdbg_ln("Got data from radio, prepare for relay. size: %d",data.size());
  
      relay = this->rxPacket->prepareForRelaying(&this->router.getFilter(), data); //change to call a function on router that filters for you, filter is private
      if (relay) {
        //TODO: this callback is causing an issue, needs to be fixed for mamaduck to get packet data
        //recvDataCallback(this->rxPacket->getBuffer());
        loginfo_ln("handleReceivedPacket: packet RELAY START");
        // NOTE:
        // Ducks will only handle received message one at a time, so there is a chance the
        // packet being sent below will never be received, especially if the cluster is small
        // there are not many alternative paths to reach other mama ducks that could relay the packet.
        
        CdpPacket packet = CdpPacket(this->rxPacket->getBuffer());
  
        //Check if Duck is desitination for this packet before relaying
        if (duckutils::isEqual(BROADCAST_DUID, packet.dduid)) {
          switch(packet.topic) {
            case reservedTopic::rreq: {
                loginfo_ln("RREQ received from %s. Updating RREQ!",
                           packet.sduid.data());
                ArduinoJson::JsonDocument rreqDoc;
                deserializeJson(rreqDoc, packet.data);
                DuckPacket::UpdateRREQ(rreqDoc, this->deviceId);
                loginfo_ln("handleReceivedPacket: RREQ updated with current DUID: %s", this->deviceId);
                //Serialize the updated RREQ packet
                std::string strRREQ;
                serializeJson(rreqDoc, strRREQ);
                this->rxPacket->getBuffer() = duckutils::stringToByteVector(strRREQ);
                err = this->duckRadio.relayPacket(this->rxPacket);
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
  
              err = this->duckRadio.relayPacket(this->rxPacket);
              if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
              } else {
                loginfo_ln("handleReceivedPacket: packet RELAY DONE");
              }
            break;
            default:
              err = this->duckRadio.relayPacket(this->rxPacket);
              if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
              } else {
                loginfo_ln("handleReceivedPacket: packet RELAY DONE");
              }
          }
        } else if(duckutils::isEqual(this->duid, packet.dduid)) { //Target device check
            std::vector<uint8_t> dataPayload;
            uint8_t num = 1;
          
          switch(packet.topic) {
              case reservedTopic::rreq: {
                  loginfo_ln("RREQ received. Updating RREQ!");
                  ArduinoJson::JsonDocument rreqDoc;
                  deserializeJson(rreqDoc, packet.data);
                  loginfo_ln("handleReceivedPacket: Sending RREP");
                  //Serialize the updated RREQ packet
                  std::string strRREP;
                  serializeJson(rreqDoc, strRREP);
                  this->rxPacket->getBuffer() = duckutils::stringToByteVector(strRREP);
                  err = this->duckRadio.relayPacket(this->rxPacket);
                  if (err != DUCK_ERR_NONE) {
                      logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d", err);
                  } else {
                      loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                  }
  
                  err = this->txPacket->prepareForSending(&this->router.getFilter(), PAPADUCK_DUID,
                    DuckType::MAMA, reservedTopic::rreq, dataPayload);
  
                  return;
              }
            break;
            default:
              err = this->duckRadio.relayPacket(this->rxPacket);
              if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
              } else {
                loginfo_ln("handleReceivedPacket: packet RELAY DONE");
              }
          }
  
        } else {
          err = this->duckRadio.relayPacket(this->rxPacket);
          if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
        }
  
      }
    }
    this->rxPacket->reset();
  }

};

#endif