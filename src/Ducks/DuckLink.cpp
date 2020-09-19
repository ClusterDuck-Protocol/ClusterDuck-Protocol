#include "../DuckLink.h"

void DuckLink::setup() {
    Duck::setup();
}

int DuckLink::run() {
  handleOtaUpdate();
  processPortalRequest();
  return 0;
}
