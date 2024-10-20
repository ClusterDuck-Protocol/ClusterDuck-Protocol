#include "../DuckLink.h"

int DuckLink::setupWithDefaults(std::array<byte,8> deviceId, std::string ssid,
                                std::string password) {
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }
  
  err = setupWifi("DuckLink");
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }

  err = setupWebServer(true);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWithDefaults rc = %d",err);
    return err;
  }
  
  loginfo_ln("DuckLink setup done");
  return DUCK_ERR_NONE;
}

void DuckLink::run() {
  Duck::logIfLowMemory();

  duckRadio.serviceInterruptFlags();

  processPortalRequest();
}
