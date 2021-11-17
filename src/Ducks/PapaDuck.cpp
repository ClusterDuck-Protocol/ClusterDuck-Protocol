#include "../PapaDuck.h"

int PapaDuck::setupWithDefaults(std::vector<byte> deviceId, String ssid,
  String password) {
  loginfo("setupWithDefaults...");

  int err = Duck::setupWithDefaults(deviceId, ssid, password);

  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults  rc = " + String(err));
    return err;
  }

  std::string name(deviceId.begin(),deviceId.end());

  err = setupWifi(name.c_str());
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults  rc = " + String(err));
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults  rc = " + String(err));
    return err;
  }



  err = setupWebServer();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults  rc = " + String(err));
    return err;
  }

  err = setupOTA();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults  rc = " + String(err));
    return err;
  }

  if (ssid.length() != 0 && password.length() != 0) {
    err = setupInternet(ssid, password);

    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults  rc = " + String(err));
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
      logerr("ERROR setupWithDefaults  rc = " + String(err));
     
      return err;
    }
    
    
  }



  loginfo("setupWithDefaults done");
  return DUCK_ERR_NONE;
}

void PapaDuck::run() {
  Duck::logIfLowMemory();

  duckRadio.serviceInterruptFlags();

  handleOtaUpdate();
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

  loginfo("handleReceivedPacket() START");
  std::vector<byte> data;
  int err = duckRadio.readReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    logerr("ERROR handleReceivedPacket. Failed to get data. rc = " +
     String(err));
    return;
  }
  // ignore pings
  if (data[TOPIC_POS] == reservedTopic::ping) {
    rxPacket->reset();
    return;
  }
  // build our RX DuckPacket which holds the updated path in case the packet is relayed
  bool relay = rxPacket->prepareForRelaying(&filter, data);
  if (relay) {
    logdbg("relaying:  " +
      duckutils::convertToHex(rxPacket->getBuffer().data(),
        rxPacket->getBuffer().size()));
    loginfo("invoking callback in the duck application...");
    
    recvDataCallback(rxPacket->getBuffer());

    if (acksEnabled) {
      const CdpPacket packet = CdpPacket(rxPacket->getBuffer());
      if (needsAck(packet)) {
        handleAck(packet);
      }
    }

    loginfo("handleReceivedPacket() DONE");
  }
}

void PapaDuck::handleAck(const CdpPacket & packet) {
  if (ackTimer.empty()) {
    logdbg("Starting new ack broadcast timer with a delay of " +
      String(timerDelay) + " ms");
    ackTimer.in(timerDelay, ackHandler, this);
  }

  storeForAck(packet);

  if (ackBufferIsFull()) {
    logdbg("Ack buffer is full. Sending broadcast ack immediately.");
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
    logdbg("Sending ack to DUID " + duckutils::toString(duid)
      + " for MUID " + duckutils::toString(muid));
    dataPayload.insert(dataPayload.end(), duid.begin(), duid.end());
    dataPayload.insert(dataPayload.end(), muid.begin(), muid.end());
  }

  int err = txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::PAPA,
    reservedTopic::ack, dataPayload);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR handleReceivedPacket. Failed to prepare ack. Error: " +
      String(err));
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    CdpPacket packet = CdpPacket(txPacket->getBuffer());
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  } else {
    logerr("ERROR handleReceivedPacket. Failed to send ack. Error: " +
      String(err));
  }

  ackStore.clear();
}

void PapaDuck::sendCommand(byte cmd, std::vector<byte> value) {
  loginfo("Initiate sending command");
  std::vector<byte> dataPayload;
  dataPayload.push_back(cmd);
  dataPayload.insert(dataPayload.end(), value.begin(), value.end());

  int err = txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::PAPA,
    reservedTopic::cmd, dataPayload);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR handleReceivedPacket. Failed to prepare ack. Error: " +
      String(err));
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    CdpPacket packet = CdpPacket(txPacket->getBuffer());
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  } else {
    logerr("ERROR handleReceivedPacket. Failed to send ack. Error: " +
      String(err));
  }
}

void PapaDuck::sendCommand(byte cmd, std::vector<byte> value, std::vector<byte> dduid) {
  loginfo("Initiate sending command");
  std::vector<byte> dataPayload;
  dataPayload.push_back(cmd);
  dataPayload.insert(dataPayload.end(), value.begin(), value.end());

  int err = txPacket->prepareForSending(&filter, dduid, DuckType::PAPA,
    reservedTopic::cmd, dataPayload);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR handleReceivedPacket. Failed to prepare cmd. Error: " +
      String(err));
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    CdpPacket packet = CdpPacket(txPacket->getBuffer());
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  } else {
    logerr("ERROR handleReceivedPacket. Failed to send cmd. Error: " +
      String(err));
  }
}

int PapaDuck::reconnectWifi(String ssid, String password) {
#ifdef CDPCFG_WIFI_NONE
  logwarn("WARNING reconnectWifi skipped, device has no WiFi.");
  return DUCK_ERR_NONE;
#else
  if (!duckNet->ssidAvailable(ssid)) {
    return DUCKWIFI_ERR_NOT_AVAILABLE;
  }
  duckNet->setupInternet(ssid, password);
  duckNet->setupDns();
  if (WiFi.status() != WL_CONNECTED) {
    logerr("ERROR WiFi reconnection failed!");
    return DUCKWIFI_ERR_DISCONNECTED;
  }
  return DUCK_ERR_NONE;
#endif
}
