#include "../PapaDuck.h"
#include <cassert>
int PapaDuck::setupWithDefaults(std::array<byte,8> deviceId, std::string ssid, std::string password) {
  loginfo_ln("setupWithDefaults...");

  int err = Duck::setupWithDefaults(deviceId, ssid, password);

  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %s",err);
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults  rc = %d",err);
    return err;
  }

  std::string name(deviceId.begin(),deviceId.end());

  err = setupWifi(name.c_str());
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults  rc = %d",err);
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults  rc = %d",err);
    return err;
  }



  err = setupWebServer();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults  rc = %d",err);
    return err;
  }

  if (ssid.length() != 0 && password.length() != 0) {
    err = setupInternet(ssid, password);

    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults  rc = %d",err);
      return err;
    }
  } 

  duckNet->loadChannel();

  if (ssid.length() == 0 && password.length() == 0) {
  // If WiFi credentials inside the INO are empty use the saved credentials
  // TODO: if the portal credentials were saved and the INO contains credentials it won't
  // take the Portal credentials on setup
    err = duckNet->loadWiFiCredentials();

    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults  rc = %d",err);
     
      return err;
    }
    
    
  }



  loginfo_ln("setupWithDefaults done");
  return DUCK_ERR_NONE;
}

void PapaDuck::run() {
  Duck::logIfLowMemory();

  duckRadio.serviceInterruptFlags();

  if (DuckRadio::getReceiveFlag()) {
    handleReceivedPacket();
    rxPacket->reset(); // TODO(rolsen): Make rxPacket local to handleReceivedPacket
  }

  // TODO(rolsen): Enforce mutually exclusive access to duckRadio.
  // ackTimer.tick() calls broadcastAck, which calls duckRadio. Since duckRadio
  // is a shared resource, we should synchronize everything in ackTimer.tick()
  // so the thread in AsyncWebServer cannot modify duckRadio while broadcastAck
  // is also modifying duckRadio.
  ackTimer.tick();
}

void PapaDuck::handleReceivedPacket() {

  loginfo_ln("handleReceivedPacket() START");
  std::vector<byte> data;
  int err = duckRadio.readReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR handleReceivedPacket. Failed to get data. rc = %d",err);
    return;
  }
  
  if (data[TOPIC_POS] == reservedTopic::ping) {
    loginfo_ln("PING received. Sending PONG!");
    err = sendPong();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR failed to send pong message. rc = %d",err);
    }
  } else if (data[TOPIC_POS] == reservedTopic::pong) {
    loginfo_ln("PONG received. Ignoring!");
  } else {
    // build our RX DuckPacket which holds the updated path in case the packet is relayed
    bool relay = rxPacket->prepareForRelaying(&filter, data);
    if (relay) {
      logdbg_ln("relaying:  %s", duckutils::convertToHex(rxPacket->getBuffer().data(), rxPacket->getBuffer().size()).c_str());
      loginfo_ln("invoking callback in the duck application...");
      
      recvDataCallback(rxPacket->getBuffer());
      
      if (acksEnabled) {
        const CdpPacket packet = CdpPacket(rxPacket->getBuffer());
        if (needsAck(packet)) {
          handleAck(packet);
        }
      }
    }
  }

  loginfo_ln("handleReceivedPacket() DONE");
}

void PapaDuck::handleAck(const CdpPacket & packet) {
  if (ackTimer.empty()) {
    logdbg_ln("Starting new ack broadcast timer with a delay of %d ms", timerDelay);
    ackTimer.in(timerDelay, ackHandler, this);
  }

  storeForAck(packet);

  if (ackBufferIsFull()) {
    logdbg_ln("Ack buffer is full. Sending broadcast ack immediately.");
    ackTimer.cancel();
    broadcastAck();
  }
}

void PapaDuck::enableAcks(bool enable) {
  acksEnabled = enable;
}

bool PapaDuck::ackHandler(PapaDuck * duck)
{
  duck->broadcastAck();
  return false;
}

void PapaDuck::storeForAck(const CdpPacket & packet) {
  ackStore.push_back(std::pair<Duid, Muid>(packet.sduid, packet.muid));
}

bool PapaDuck::ackBufferIsFull() {
  return (ackStore.size() >= MAX_MUID_PER_ACK);
}

bool PapaDuck::needsAck(const CdpPacket & packet) {
  if (packet.topic == reservedTopic::ack) {
    return false;
  } else {
    return true;
  }
}

void PapaDuck::broadcastAck() {
  assert(ackStore.size() <= MAX_MUID_PER_ACK);

  const byte num = static_cast<byte>(ackStore.size());

  std::vector<byte> dataPayload;
  dataPayload.push_back(num);
  for (int i = 0; i < num; i++) {
    Duid duid = ackStore[i].first;
    Muid muid = ackStore[i].second;
    logdbg_ln("Storing ack for duid: %s, muid: %s",
      duckutils::convertToHex(duid.data(), DUID_LENGTH).c_str(),
      duckutils::convertToHex(muid.data(), MUID_LENGTH).c_str());
      
    dataPayload.insert(dataPayload.end(), duid.begin(), duid.end());
    dataPayload.insert(dataPayload.end(), muid.begin(), muid.end());
  }

  int err = txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::PAPA,
    reservedTopic::ack, dataPayload);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR handleReceivedPacket. Failed to prepare ack. Error: %d",err);
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    CdpPacket packet = CdpPacket(txPacket->getBuffer());
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  } else {
    logerr_ln("ERROR handleReceivedPacket. Failed to send ack. Error: %d",err);
  }

  ackStore.clear();
}

void PapaDuck::sendCommand(byte cmd, std::vector<byte> value) {
  loginfo_ln("Initiate sending command");
  std::vector<byte> dataPayload;
  dataPayload.push_back(cmd);
  dataPayload.insert(dataPayload.end(), value.begin(), value.end());

  int err = txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::PAPA,
    reservedTopic::cmd, dataPayload);
  if (err != DUCK_ERR_NONE) {
    
    logerr_ln("ERROR handleReceivedPacket. Failed to prepare ack. Error: %d",err);
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    CdpPacket packet = CdpPacket(txPacket->getBuffer());
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  } else {
    logerr_ln("ERROR handleReceivedPacket. Failed to send ack. Error: %d", err);
  }
}

void PapaDuck::sendCommand(byte cmd, std::vector<byte> value, std::array<byte,8> dduid) {
  loginfo_ln("Initiate sending command");
  std::vector<byte> dataPayload;
  dataPayload.push_back(cmd);
  dataPayload.insert(dataPayload.end(), value.begin(), value.end());

  int err = txPacket->prepareForSending(&filter, dduid, DuckType::PAPA,
    reservedTopic::cmd, dataPayload);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR handleReceivedPacket. Failed to prepare cmd. Error: %d",err);
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    CdpPacket packet = CdpPacket(txPacket->getBuffer());
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  } else {
    logerr_ln("ERROR handleReceivedPacket. Failed to send cmd. Error: %d",err);
  }
}

int PapaDuck::reconnectWifi(std::string ssid, std::string password) {
#ifdef CDPCFG_WIFI_NONE
  logwarn_ln("WARNING reconnectWifi skipped, device has no WiFi.");
  return DUCK_ERR_NONE;
#else
  duckNet->setupInternet(ssid, password);
  duckNet->setupDns();
  if (WiFi.status() != WL_CONNECTED) {
    logerr_ln("ERROR WiFi reconnection failed!");
    return DUCKWIFI_ERR_DISCONNECTED;
  }
  return DUCK_ERR_NONE;
#endif
}