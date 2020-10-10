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
  /*
   int pSize = duckLora->storePacketData();
   if (pSize > 0) {
     for (int i = 0; i < pSize; i++) {
       if (duckLora->getTransmitedByte(i) == iamhere_B) {
         Serial.println("[DuckDetect] run() - got ping response!");
         rssiCb(duckLora->getRSSI());
       }
     }
   }
   */
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

// TODO: implement with new packet format 
void DuckDetect::sendPing(bool startReceive) {
  
}