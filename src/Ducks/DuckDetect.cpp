#include "../DuckDetect.h"

void DuckDetect::setupWithDefaults() {
  setupRadio();
  /*
  setupWifi(false);
  setupDns();
  setupWebServer(false);
  setupOTA();
  */
}

int DuckDetect::run() {
  int val = 0;
  handleOtaUpdate();
  if (receivedFlag) {
    receivedFlag = false;
    duckutils::setDuckInterrupt(false);
    int pSize = duckLora->handlePacket();
    Serial.print("DuckDetect] run pSize ");
    Serial.println(pSize);
    rxCb(duckLora->getTransmissionBuffer());

    /*
    if (pSize > 0) {
      for (int i = 0; i < pSize; i++) {
        if (duckLora->getTransmitedByte(i) == iamhere_B) {
          val = duckLora->getRSSI();
        }
      }
    }
    */
    duckutils::setDuckInterrupt(true);
    Serial.println("DuckDetect] run startReceive");
    startReceive();
  }
  return val;
}


void DuckDetect::sendPing(bool startReceive) {
  duckLora->couple(ping_B, "0");
  startTransmit();
  
  if (startReceive) {
    this->startReceive();
    }
}