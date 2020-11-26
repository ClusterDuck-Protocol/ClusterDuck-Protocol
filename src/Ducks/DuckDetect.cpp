#include "../DuckDetect.h"

int DuckDetect::setupWithDefaults(std::vector<byte> deviceId, String ssid,
                                  String password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults - setupRadio rc = " + String(err));
    return err;
  }

  if (!ssid.length() != 0 && !password.length() != 0) {
    err = setupWifi();
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults - setupWifi rc = " + String(err));
      return err;
    }

    err = setupDns();
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults - setupDns rc = " + String(err));
      return err;
    }

    err = setupWebServer(true);
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults - setupWebServer rc = " + String(err));
      return err;
    }

    err = setupOTA();
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR setupWithDefaults - setupOTA src = " + String(err));
      return err;
    }
  }
  loginfo("DuckDetect setup done");
  return DUCK_ERR_NONE;
}

void DuckDetect::run() {
  handleOtaUpdate();
  if (getReceiveFlag()) {
    setReceiveFlag(false);
    duckutils::setInterrupt(false);

    handleReceivedPacket();

    duckutils::setInterrupt(true);
    startReceive();
  }
}

void DuckDetect::handleReceivedPacket() {

  loginfo("handleReceivedPacket()...");

  std::vector<byte> data;
  int err = duckRadio->readReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Failed to get data from DuckRadio. rc = " + String(err));
    return;
  }

  if (data[TOPIC_POS] == reservedTopic::pong) {
    logdbg("run() - got ping response!");
    rssiCb(duckRadio->getRSSI());
  }
}

void DuckDetect::sendPing(bool startReceive) {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  err = txPacket->prepareForSending(ZERO_DUID, DuckType::DETECTOR, reservedTopic::ping, data);

  if (err == DUCK_ERR_NONE) {
    err = duckRadio->sendData(txPacket->getBuffer());
    if (startReceive) {
      duckRadio->startReceive();
    }
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR Failed to ping, err = " + String(err));
    }
  } else {
    logerr("ERROR Failed to build packet, err = " + String(err));
  }
}