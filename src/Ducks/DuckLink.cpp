#include "../DuckLink.h"

void DuckLink::setupWithDefaults() {
    Duck::setupWithDefaults();
    setupRadio();
}

void DuckLink::run() {
  handleOtaUpdate();
  processPortalRequest();
}
