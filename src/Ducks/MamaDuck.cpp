#include "../MamaDuck.h"

void MamaDuck::setupWithDefaults(String ssid, String password) {
  Duck::setupWithDefaults(ssid, password);
  setupRadio();

  setupWifi();
  setupDns();
  setupWebServer(true);
  setupOTA();

  Serial.println("MamaDuck setup done");

  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);
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