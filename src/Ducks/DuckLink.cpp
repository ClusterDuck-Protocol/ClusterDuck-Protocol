#include "../DuckLink.h"

int DuckLink::setupWithDefaults(std::vector<byte> deviceId, String ssid,
                                String password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupWithDefaults rc = " + String(err));
    return err;
  }
  
  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupWithDefaults rc = " + String(err));
    return err;
  }
  if( !ssid.length() == 0 && !password.length() == 0) {
    err = setupWifi();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupDns();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupWebServer(true);
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }

    err = setupOTA();
    if (err != DUCK_ERR_NONE) {
      logerr_ln("setupWithDefaults rc = " + String(err));
      return err;
    }
  }
  loginfo_ln("DuckLink setup done");
  return DUCK_ERR_NONE;
}

void DuckLink::run() {
  duckRadio->processRadioIrq();
  handleOtaUpdate();
  processPortalRequest();
}
