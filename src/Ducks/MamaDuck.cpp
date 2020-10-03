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

  CDP_Packet packet;
  int pSize = duckLora->getReceivedPacket(&packet);

  if (pSize > 0) {
    if (packet.path.size() == MAX_PATH_LENGTH) {
      Serial.println("Oops. Max hops reached. Packet may be lost");
      return;
    }
    
    String paths = duckutils::convertToHex(packet.path.data(), packet.path.size());
    if (paths.indexOf(deviceId) < 0) {
      Serial.println("mama duck duid not found in path. adding it");
      packet.path.insert(packet.path.end(), deviceId.begin(), deviceId.end());
    }
    Serial.print("PATH = ");
    Serial.println(duckutils::convertToHex(packet.path.data(), packet.path.size()));
  } else {
    // discard any unrecognized packets
    Serial.println("[MamaDuck] handleReceivedMessage() messaage is empty");

    duckLora->resetTransmissionBuffer();
  }
}

/*
void MamaDuck::handleReceivedMessage() {

  CDP_Packet packet;
  int pSize = duckLora->getReceivedPacket(&packet);
  // read the lora message and transfer it to the transmission buffer
  //int pSize = duckLora->storePacketData();
  Serial.print("[MamaDuck] handleReceivedMessage() pSize = ");
  Serial.println(pSize);
  if (pSize > 0) {
    // These 2 methods should be combined in one.
    // the string returned by getPacketData() is only the message topic
    // it could instead return the whole packet and we can get the topic
    // from the Packet data structure
    String msg = duckLora->getPacketData(pSize);
    Packet lastPacket = duckLora->getLastPacket();
    duckLora->resetPacketIndex();

    if (msg != "ping" && !idInPath(lastPacket.path)) {
      Serial.print("[MamaDuck] handleReceivedMessage() send: ");
      Serial.println(msg);

      duckLora->sendPayloadStandard(lastPacket.payload, lastPacket.topic,
                                    lastPacket.senderId, lastPacket.messageId,
                                    lastPacket.path);
      duckLora->resetTransmissionBuffer();
    }
  } else {
    // discard any unrecognized packets
    duckLora->resetTransmissionBuffer();
  }
}
*/

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