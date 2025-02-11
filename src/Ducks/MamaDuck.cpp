#include "../MamaDuck.h"
#include "../MemoryFree.h"


int MamaDuck::setupWithDefaults(std::array<byte,8> deviceId, std::string ssid, std::string password) {
  
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

  std::string name(deviceId.begin(),deviceId.end());
  err = setupWifi(name.c_str());

  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupWebServer(false);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);

  duckNet->loadChannel();

  return DUCK_ERR_NONE;
}

void MamaDuck::run() {
  Duck::logIfLowMemory();

  duckRadio.serviceInterruptFlags();

  if (DuckRadio::getReceiveFlag()) {
    
    handleReceivedPacket();
    rxPacket->reset();
  }
  processPortalRequest();
}

void MamaDuck::handleReceivedPacket() {
 
  std::vector<byte> data;
  bool relay = false;
  
  loginfo_ln("====> handleReceivedPacket: START");

  int err = duckRadio.readReceivedData(&data);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR failed to get data from DuckRadio. rc = %d",err);
    return;
  }
  logdbg_ln("Got data from radio, prepare for relay. size: %d",data.size());

  relay = rxPacket->prepareForRelaying(&filter, data);
  if (relay) {
    //TODO: this callback is causing an issue, needs to be fixed for mamaduck to get packet data
    //recvDataCallback(rxPacket->getBuffer());
    loginfo_ln("handleReceivedPacket: packet RELAY START");
    // NOTE:
    // Ducks will only handle received message one at a time, so there is a chance the
    // packet being sent below will never be received, especially if the cluster is small
    // there are not many alternative paths to reach other mama ducks that could relay the packet.
    
    CdpPacket packet = CdpPacket(rxPacket->getBuffer());

    //Check if Duck is desitination for this packet before relaying
    if (duckutils::isEqual(BROADCAST_DUID, packet.dduid)) {
      switch(packet.topic) {
        case reservedTopic::ping:
          loginfo_ln("PING received. Sending PONG!");
          err = sendPong();
          if (err != DUCK_ERR_NONE) {
            logerr_ln("ERROR failed to send pong message. rc = %d",err);
          }
          return;
        break;
        case reservedTopic::pong:
          loginfo_ln("PONG received. Ignoring!");
        break;
        case reservedTopic::cmd:
          loginfo_ln("Command received");
          handleCommand(packet);

          err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
        break;
        default:
          err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
      }
    } else if(duckutils::isEqual(duid, packet.dduid)) { //Target device check
        std::vector<byte> dataPayload;
        byte num = 1;
      
      switch(packet.topic) {
        case topics::dcmd:
          loginfo_ln("Duck command received");
          handleDuckCommand(packet);
        break;
        case reservedTopic::cmd:
          loginfo_ln("Command received");
          
          //Handle Command
          handleCommand(packet);

        break;
        default:
          err = duckRadio.relayPacket(rxPacket);
          if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
          } else {
            loginfo_ln("handleReceivedPacket: packet RELAY DONE");
          }
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
}

void MamaDuck::handleCommand(const CdpPacket & packet) {
  int err;
  std::vector<byte> dataPayload;
  std::vector<byte> alive {'I','m',' ','a','l','i','v','e'};

  switch(packet.data[0]) {
    case 0:
      //Send health quack
      loginfo_ln("Health request received");
      dataPayload.insert(dataPayload.end(), alive.begin(), alive.end());
      err = txPacket->prepareForSending(&filter, PAPADUCK_DUID, 
        DuckType::MAMA, topics::health, dataPayload);
      if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR handleReceivedPacket. Failed to prepare ack. Error: %d", err);
      }

      err = duckRadio.sendData(txPacket->getBuffer());
      if (err == DUCK_ERR_NONE) {
        CdpPacket healthPacket = CdpPacket(txPacket->getBuffer());
        filter.bloom_add(healthPacket.muid.data(), MUID_LENGTH);
      } else {
        logerr_ln("ERROR handleReceivedPacket. Failed to send ack. Error: %d", err);
      }

    break;
    case 1:
      //Change wifi status
      #ifdef CDPCFG_WIFI_NONE
        logwarn_ln("WiFi not supported");
      #else
        if((char)packet.data[1] == '1') {
          loginfo_ln("Command WiFi ON");
          WiFi.mode(WIFI_AP);

        } else if ((char)packet.data[1] == '0') {
          loginfo_ln("Command WiFi OFF");
          WiFi.mode( WIFI_MODE_NULL );
        }
      #endif
    break;
    default:
      logerr_ln("Command not recognized");
  }

}

void MamaDuck::handleDuckCommand(const CdpPacket & packet) {
  loginfo_ln("Doesn't do anything yet. But Duck Command was received.");
}

bool MamaDuck::getDetectState() { return duckutils::getDetectState(); }
