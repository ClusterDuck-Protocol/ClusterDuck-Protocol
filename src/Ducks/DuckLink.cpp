#include "../DuckLink.h"

void DuckLink::setupWithDefaults(String ssid, String password) {
  Duck::setupWithDefaults(ssid, password);
  setupRadio();
}

void DuckLink::run() {
  handleOtaUpdate();
  processPortalRequest();
}
