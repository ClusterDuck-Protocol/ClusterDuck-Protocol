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

    err = setupWifi("PapaDuck Setup");
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults  rc = " + String(err));
      return err;
    }

    err = setupDns();
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

    err = setupWebServer(false);
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults  rc = " + String(err));
      return err;
    }

    err = setupOTA();
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults  rc = " + String(err));
      return err;
    }
  loginfo("setupWithDefaults done");
  return DUCK_ERR_NONE;
}

void PapaDuck::run() {

  handleOtaUpdate();
  if (getReceiveFlag()) {
    duckutils::setInterrupt(false);
    setReceiveFlag(false);

    handleReceivedPacket();
    rxPacket->reset();
    
    duckutils::setInterrupt(true);
    startReceive();
  }
}

void PapaDuck::handleReceivedPacket() {

  loginfo("handleReceivedPacket() START");
  std::vector<byte> data;
  int err = duckRadio->readReceivedData(&data);

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
  bool relay = rxPacket->prepareForRelaying(duid, data);
  if (relay) {
    logdbg("relaying:  " +
            duckutils::convertToHex(rxPacket->getBuffer().data(),
                                    rxPacket->getBuffer().size()));
    loginfo("invoking callback in the duck application...");
    recvDataCallback(rxPacket->getBuffer());
    loginfo("handleReceivedPacket() DONE");
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
