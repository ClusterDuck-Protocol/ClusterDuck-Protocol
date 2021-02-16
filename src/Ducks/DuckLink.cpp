#include "../DuckLink.h"
#include "../include/DuckCrypto.h"

DuckCrypto test;

int DuckLink::setupWithDefaults(std::vector<byte> deviceId, String ssid,
                                String password) {
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
  
  err = setupWifi("DuckLink");
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
  
  loginfo("DuckLink setup done");
  return DUCK_ERR_NONE;
}

void DuckLink::crypto(std::vector<uint8_t> text, std::vector<uint8_t> encryptedData, size_t inc) {
  test.encryptData(text, encryptedData, inc);
}

void DuckLink::run() {
  duckRadio->processRadioIrq();
  handleOtaUpdate();
  processPortalRequest();
}
