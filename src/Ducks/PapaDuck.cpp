#include "../PapaDuck.h"

void PapaDuck::setupWithDefaults() {
  Duck::setupWithDefaults();
  setupRadio();
  setupWifi("PapaDuck Setup");
  setupDns();
  setupWebServer(false);
  setupOTA();
  Serial.println("PapaDuck Online");
}

void PapaDuck::run() {

  handleOtaUpdate();
  if (getReceiveFlag()) {
    setReceiveFlag(false);
    duckutils::setDuckInterrupt(false);
    int pSize = duckLora->handlePacket();
    // FIXME: This needs a design review.
    // What message topics Papa Duck is supposed to send out
    // what is this pSize > 3 supposed to achieve
    if (pSize > 3) {
      recvDataCallback(duckLora->getLastPacket());
    }
    duckutils::setDuckInterrupt(true);
    startReceive();
  }
}

int PapaDuck::reconnectWifi(String ssid, String password) {
    if (!duckNet->ssidAvailable(ssid)) {
      return DUCKWIFI_ERR_NOT_AVAILABLE;
    }
    duckNet->setupInternet(ssid, password);
    duckNet->setupDns();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("[PapaDuck] WiFi reconnection failed!");
        return DUCKWIFI_ERR_DISCONNECTED;
    }

    return DUCK_ERR_NONE;
}