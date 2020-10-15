#include "../DuckDetect.h"

int DuckDetect::setupWithDefaults(std::vector<byte> deviceId, String ssid,
                                String password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    Serial.println("[DuckDetect] setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    Serial.println("[DuckDetect] setupWithDefaults rc = " + String(err));
    return err;
  }

  if (!ssid.isEmpty() && !password.isEmpty()) {
    err = setupWifi();
    if (err != DUCK_ERR_NONE) {
      Serial.println("[DuckDetect] setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupDns();
    if (err != DUCK_ERR_NONE) {
      Serial.println("[DuckDetect] setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupWebServer(true);
    if (err != DUCK_ERR_NONE) {
      Serial.println("[DuckDetect] setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupOTA();
    if (err != DUCK_ERR_NONE) {
      Serial.println("[DuckDetect] setupWithDefaults rc = " + String(err));
      return err;
    }
  }
  Serial.println("DuckDetect setup done");
  return DUCK_ERR_NONE;
}

void DuckDetect::handleReceivedPacket() {

  Serial.println("[DuckDetect] handleReceivedPacket()...");

  std::vector<byte> data;
  int err = duckLora->getReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    Serial.print("[DuckDetect] ERROR - failed to get data from DuckRadio. rc = ");
    Serial.println(err);
    return;
  }

  if (data[TOPIC_POS] == reservedTopic::pong) {
    Serial.println("[DuckDetect] run() - got ping response!");
    rssiCb(duckLora->getRSSI());
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
    err = duckLora->sendData(txPacket->getDataByteBuffer(),
                             txPacket->getBufferLength());
    if (startReceive) {
      duckLora->startReceive();
    }
    if (err != DUCK_ERR_NONE) {
      Serial.println("[DuckDetect] Oops! failed to ping, err = " + String(err));
    }
  } else {
    Serial.println("[DuckDetect] Oops! failed to build packet, err = "+ String(err));
    return;
  }
}