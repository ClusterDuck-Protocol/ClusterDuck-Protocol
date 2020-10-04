#include "../MamaDuck.h"

bool MamaDuck::idInPath(String path) {
  Serial.print("[MamaDuck] idInPath '");
  Serial.print(path);
  Serial.print("' ");
  String temp = "";
  int len = path.length() + 1;
  char arr[len];
  path.toCharArray(arr, len);

  for (int i = 0; i < len; i++) {
    if (arr[i] == ',' || i == len - 1) {
      if (temp == deviceId) {
        Serial.println("true");
        return true;
      }
      temp = "";
    } else {
      temp += arr[i];
    }
  }
  Serial.println("false");
  return false;
}

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

void MamaDuck::handleReceivedMessage() {

  Serial.println("[MamaDuck] handleReceivedMessage()...");
  
  // start with a clean rx duck packet
  // get  data from the lora driver
  // update the rx duck packet
  // add our duid to the path if not already present
  // send the message back into the mesh if it needs to be relayed
  rxPacket->reset();
  std::vector<byte> data;
  int err = duckLora->getReceivedData(&data);
  if (err != DUCK_ERR_NONE) {
    Serial.print("[MamaDuck] >> could not get received data from DuckLora. err = ");
    Serial.println(err);
    return;
  }

  bool relay = rxPacket->update(duid, data);
  if (relay) {
    Serial.println(rxPacket->getPathAsHexString());
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
    handleReceivedMessage();

    duckutils::setDuckInterrupt(true);
    startReceive();
  }
  processPortalRequest();
}