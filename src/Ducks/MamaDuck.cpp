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

void MamaDuck::setup() {
  Duck::setup();
  setupRadio();
  setupWifi();
  setupDns();
  setupWebServer(true);
  setupOTA();

  Serial.println("MamaDuck Online");

  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);
}

int MamaDuck::run() {

  handleOtaUpdate();

  if (duckutils::getDuckInterrupt()) {
    duckutils::getTimer().tick();
  }

  // Mama ducks can also receive packets from other ducks
  // Here we check whether a packet needs to be relayed or not
  // For safe processing of the received packet we make sure
  // to disable interrupts, before handling the received packet.
  if (receivedFlag) {
    receivedFlag = false;
    duckutils::setDuckInterrupt(false);
    int pSize = duckLora->handlePacket();
    Serial.print("[MamaDuck] run rcv packet. pSize = ");
    Serial.println(pSize);

    if (pSize > 0) {
      // These 2 methods should be combined in one.
      // the string returned by getPacketData() is only the message topic
      // it could instead return the whole packet and we can get the topic from
      // the Packet data structure
      String msg = duckLora->getPacketData(pSize);
      Packet lastPacket = duckLora->getLastPacket();
      duckLora->resetPacketIndex();

      if (msg != "ping" && !idInPath(lastPacket.path)) {
        Serial.print("[MamaDuck] run relay packet msg: ");
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
    duckutils::setDuckInterrupt(true);
    startReceive();
  }

  processPortalRequest();
  return 0;
}