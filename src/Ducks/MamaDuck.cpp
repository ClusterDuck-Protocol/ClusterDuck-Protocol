#include "../MamaDuck.h"

int MamaDuck::setupWithDefaults(std::vector<byte> deviceId, String ssid, String password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);

  if (err != DUCK_ERR_NONE) {
    Serial.println("[MamaDuck] setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    Serial.println("[MamaDuck] setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupWifi();
  if (err != DUCK_ERR_NONE) {
    Serial.println("[MamaDuck] setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    Serial.println("[MamaDuck] setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupWebServer(true);
  if (err != DUCK_ERR_NONE) {
    Serial.println("[MamaDuck] setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupOTA();
  if (err != DUCK_ERR_NONE) {
    Serial.println("[MamaDuck] setupWithDefaults rc = " + String(err));
    return err;
  }
  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);
  Serial.println("MamaDuck setup done");
  return DUCK_ERR_NONE;
}

void MamaDuck::handleReceivedPacket() {

  Serial.println("[MamaDuck] handleReceivedPacket()...");

  rxPacket->reset();
  
  std::vector<byte> data;
  int err = duckLora->getReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    Serial.print("[MamaDuck] ERROR - failed to get data from DuckLora. rc = ");
    Serial.println(err);
    return;
  }

  bool relay = rxPacket->update(duid, data);
  if (relay) {
    Serial.println("[MamaDuck] path updated: " + rxPacket->getPathAsHexString());
    duckLora->sendData(rxPacket->getCdpPacketBuffer());
  }
}

void MamaDuck::run() {

  handleOtaUpdate();

  if (duckutils::getDuckInterrupt()) {
    duckutils::getTimer().tick();
  }

  // Mama ducks can also receive packets from other ducks
  // For safe processing of the received packet we make sure
  // to disable interrupts, before handling the received packet.
  if (getReceiveFlag()) {
    setReceiveFlag(false);
    duckutils::setDuckInterrupt(false);

    // Here we check whether a packet needs to be relayed or not
    handleReceivedPacket();

    duckutils::setDuckInterrupt(true);
    startReceive();
  }
  processPortalRequest();
}