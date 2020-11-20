#include "../MamaDuck.h"
#include "../MemoryFree.h"

int MamaDuck::setupWithDefaults(std::vector<byte> deviceId, String ssid, String password) {
  
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupWifi();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupWebServer(true);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupOTA();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }
  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);

  return DUCK_ERR_NONE;
}

void MamaDuck::handleReceivedPacket() {

  std::vector<byte> data;
  bool relay = false;
  
  //TODO: revisit packet reset calls, they should no longer be needed
  //once RadioLib corrupt packet issue is fixed
  loginfo("====> handleReceivedPacket: START");
  int err = duckRadio->readReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    logerr("ERROR failed to get data from DuckRadio. rc = "+ String(err));
    rxPacket->reset();
    return;
  }
  logdbg("Got data from radio, prepare for relay. size: "+ String(data.size()));

  relay = rxPacket->prepareForRelaying(duid, data);
  if (relay) {
    loginfo("handleReceivedPacket: packet RELAY START");
    // NOTE:
    // Ducks will only handle received message one at a time, so there is a chance the
    // packet being sent below will never be received, especially if the cluster is small
    // there are not many alternative paths to reach other mama ducks that could relay the packet.
    // We could add some kind of random delay before the message is sent, but that's not really a generic solution
    // delay(500);
    if (rxPacket->getCdpPacket().topic == reservedTopic::ping) {
      err = sendPong();
      if (err != DUCK_ERR_NONE) {
        logerr("ERROR failed to send pong message. rc = " + String(err));
      }
      rxPacket->reset();
      return;
    }

    err = duckRadio->relayPacket(rxPacket);
    if (err != DUCK_ERR_NONE) {
      logerr("====> ERROR handleReceivedPacket failed to relay. rc = " + String(err));
    } else {
      loginfo("handleReceivedPacket: packet RELAY DONE");
    }
  }
  rxPacket->reset();
}

void MamaDuck::run() {

  handleOtaUpdate();
  if (getReceiveFlag()) {
    duckutils::setInterrupt(false);
    setReceiveFlag(false);
    handleReceivedPacket();
    rxPacket->reset();
    duckutils::setInterrupt(true);
    startReceive();
  }
  processPortalRequest();
}