#include "../DuckLink.h"

void DuckLink::setup() {
    Duck::setup();
}

int DuckLink::run() {
  handleOtaUpdate();
  processPortalRequest();
  return 0;
}

int DuckLink::startTransmit() {
  int err = duckLora->transmitData();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[DuckLink] Oops! Lora transmission failed, err = ");
    Serial.print(err);
  }
  return err;
}