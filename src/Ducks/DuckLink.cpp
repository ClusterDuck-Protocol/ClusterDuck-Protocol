#include "DuckLink.h"

int DuckLink::setupWithDefaults(std::array<byte,8> deviceId, std::string ssid,
                                std::string password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }
  
  err = setupWifi("DuckLink");
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupWebServer(true);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }
  
  loginfo_ln("DuckLink setup done");
  return DUCK_ERR_NONE;
}

void DuckLink::handleReceivedPacket() {
  if (DuckRadio::getReceiveFlag()){
    std::vector<uint8_t> data;

    loginfo_ln("====> handleReceivedPacket: START");

    int err = duckRadio.readReceivedData(&data);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR failed to get data from DuckRadio. rc = %d",err);
      return;
    }
    logdbg_ln("Got data from radio, prepare for relay. size: %d",data.size());
      
    CdpPacket packet = CdpPacket(rxPacket->getBuffer());

    //Check if Duck is desitination for this packet
    if (duckutils::isEqual(BROADCAST_DUID, packet.dduid)) {
      switch(packet.topic) {
        case reservedTopic::rreq: {
            // check number of hops in packet header
            // send RREP if DUCKORIGIN is one hop away
            // if more than one hop, return
            if( packet.hopCount > 1) {
              loginfo_ln("RREQ received from %s. Hop count is %d. Not sending RREP.",
                         duckutils::toString(packet.data).c_str(), packet.hopCount);
              return;
            } else {
                txPacket->prepareForSending(&filter, duckutils::stringToArray<uint8_t,8>(deviceId),
                                            DuckType::LINK, reservedTopic::rrep,
                                            duckutils::stringToByteVector(DuckPacket::prepareRREP(this->deviceId, packet)));
                err = duckRadio.sendData(txPacket->getBuffer());
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
                } else {
                    loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
                }
            }
          break;
        }
        case reservedTopic::ping:
          loginfo_ln("PING received. Sending PONG!");
          err = sendPong();
          if (err != DUCK_ERR_NONE) {
            logerr_ln("ERROR failed to send pong message. rc = %d",err);
          }
        break;
      }
    } else if(duckutils::isEqual(duid, packet.dduid)) { //Target device check
        std::vector<uint8_t> dataPayload;
        byte num = 1;
      
      switch(packet.topic) {
        case reservedTopic::rreq: {
            // send RREP unconditionally

            txPacket->prepareForSending(&filter, duckutils::stringToArray<uint8_t,8>(deviceId),
                                        DuckType::LINK, reservedTopic::rrep,
                                        duckutils::stringToByteVector(DuckPacket::prepareRREP(this->deviceId, packet)));
            err = duckRadio.sendData(txPacket->getBuffer());
            if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to send. rc = %d", err);
            } else {
                loginfo_ln("handleReceivedPacket: RREP packet SEND DONE");
            }
        break;
      }
        case reservedTopic::rrep: {
            // check if the RREP ORIGIN is the same as the current device
            // add last hop to the path to routing table
            // then return
        }
      case reservedTopic::ping:
        loginfo_ln("PING received. Sending PONG!");
        err = sendPong();
        if (err != DUCK_ERR_NONE) {
          logerr_ln("ERROR failed to send pong message. rc = %d",err);
        }
      break;
      }

    } else {
      err = duckRadio.relayPacket(rxPacket);
      if (err != DUCK_ERR_NONE) {
        logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
      } else {
        loginfo_ln("handleReceivedPacket: packet RELAY DONE");
      }
    }

  }
  rxPacket->reset();
}
