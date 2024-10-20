#include "../DuckDetect.h"

int DuckDetect::setupWithDefaults(std::array<byte,8> deviceId, std::string ssid,
                                  std::string password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = %s\n",err);
    return err;
  }

  err = setDeviceId(deviceId);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = %s\n",err);
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults - setupRadio rc = %s\n",err);
    return err;
  }

  // if (!ssid.length() != 0 && !password.length() != 0) {
  //   err = setupWifi();
  //   if (err != DUCK_ERR_NONE) {
  //     logerr("ERROR setupWithDefaults - setupWifi rc = %s\n",err);
  //     return err;
  //   }

  // err = setupDns();
  // if (err != DUCK_ERR_NONE) {
  //   logerr("ERROR setupWithDefaults - setupDns rc = %s\n",err);
  //   return err;
  // }

  // err = setupWebServer(true);
  // if (err != DUCK_ERR_NONE) {
  //   logerr("ERROR setupWithDefaults - setupWebServer rc = %s\n",err);
  //   return err;
  // }

  duckNet->loadChannel();

  loginfo("DuckDetect setup done");
  return DUCK_ERR_NONE;
}

void DuckDetect::run() {
  Duck::logIfLowMemory();

  duckRadio.serviceInterruptFlags();

  if (DuckRadio::getReceiveFlag()) {
    handleReceivedPacket();
  }
}

void DuckDetect::handleReceivedPacket() {

  loginfo("handleReceivedPacket()...");

  std::vector<byte> data;
  int err = duckRadio.readReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Failed to get data from DuckRadio. rc = %s\n",err);
    return;
  }

  if (data[TOPIC_POS] == reservedTopic::pong) {
    logdbg("run() - got ping response!");
    rssiCb(duckRadio.getRSSI());
  }
}

void DuckDetect::sendPing(bool startReceive) {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  err = txPacket->prepareForSending(&filter, BROADCAST_DUID, DuckType::DETECTOR, reservedTopic::ping, data);

  if (err == DUCK_ERR_NONE) {
    err = duckRadio.sendData(txPacket->getBuffer());
    if (startReceive) {
      duckRadio.startReceive();
    }
    if (err != DUCK_ERR_NONE) {
      logerr("ERROR Failed to ping, err = %s\n",err);
    }
  } else {
    logerr("ERROR Failed to build packet, err = %s\n",err);
  }
}
