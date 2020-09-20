#include "../DuckLink.h"

void DuckLink::setupWithDefaults() {
    Duck::setupWithDefaults();
    setupSerial();
    setupRadio();
}

int DuckLink::run() {
  handleOtaUpdate();
  processPortalRequest();
  return 0;
}
