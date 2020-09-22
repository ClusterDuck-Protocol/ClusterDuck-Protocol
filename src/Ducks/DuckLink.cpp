#include "../DuckLink.h"

void DuckLink::setupWithDefaults(String ssid, String password) {
  Duck::setupWithDefaults(ssid, password);
  setupRadio();
  if( !ssid.isEmpty() && !password.isEmpty()) {
    setupWifi();
    setupDns();
    setupWebServer(true);
    setupOTA();
  }
  Serial.println("DuckLink setup done");
}

void DuckLink::run() {
  handleOtaUpdate();
  processPortalRequest();
}
