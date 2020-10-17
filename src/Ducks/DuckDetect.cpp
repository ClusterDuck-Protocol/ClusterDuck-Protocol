#include "../DuckDetect.h"

int DuckDetect::setupWithDefaults(std::vector<byte> deviceId, String ssid,
                                String password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupWithDefaults rc = " + String(err));
    return err;
  }

  if (!ssid.length() != 0 && !password.length() != 0) {
    err = setupWifi();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupDns();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupWebServer(true);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupOTA();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }
  }
  loginfo_ln("DuckDetect setup done");
  return DUCK_ERR_NONE;
}

void DuckDetect::handleReceivedPacket() {

  loginfo_ln("handleReceivedPacket()...");

  std::vector<byte> data;
  int err = duckRadio->getReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    logerr_ln("Failed to get data from DuckRadio. rc = " + String(err));
    return;
  }

  if (data[TOPIC_POS] == reservedTopic::pong) {
    logdbg_ln("run() - got ping response!");
    rssiCb(duckRadio->getRSSI());
  }
}

void DuckDetect::run() {
  handleOtaUpdate();
  if (getReceiveFlag()) {
    setReceiveFlag(false);
    duckutils::setDuckInterrupt(false);

    handleReceivedPacket();

    duckutils::setDuckInterrupt(true);
    startReceive();
  }
}

void DuckDetect::sendPing(bool startReceive) {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  txPacket->reset();
  err = txPacket->buildPacketBuffer(reservedTopic::ping, data);

  if (err == DUCK_ERR_NONE) {
    err = duckRadio->sendData(txPacket->getDataByteBuffer(),
                             txPacket->getBufferLength());
    if (startReceive) {
      duckRadio->startReceive();
    }
    if (err != DUCK_ERR_NONE) {
      logerr_ln("Failed to ping, err = " + String(err));
    }
  } else {
    logerr_ln("Failed to build packet, err = "+ String(err));
    return;
  }
}