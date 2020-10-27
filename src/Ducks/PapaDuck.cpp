#include "../PapaDuck.h"

void PapaDuck::setupWithDefaults(String ssid, String password) {
  Duck::setupWithDefaults(ssid, password);
  setupRadio();

  if (ssid.length() != 0 && password.length() != 0) {
    setupWifi("PapaDuck Setup");
    setupDns();
    setupInternet(ssid, password);
    setupWebServer(false);
    setupOTA();
  }
  Serial.println("PapaDuck setup done");
}

void PapaDuck::run() {

  handleOtaUpdate();
  if (getReceiveFlag()) {
    setReceiveFlag(false);
    duckutils::setDuckInterrupt(false);
    int pSize = duckLora->handlePacket();
    // FIXME: This needs a design review.
    // What message topics Papa Duck is supposed to send out?
    // what is this pSize > 3 supposed to achieve ?
    if (pSize > 3) {
      // Feed the data into our internal buffer. 
      // FIXME: This is lame! The data should already in the buffer the moment
      // we receive somethng from the lora module.
      duckLora->getPacketData(pSize);
      recvDataCallback(duckLora->getLastPacket());
    }
    duckutils::setDuckInterrupt(true);
    startReceive();
  }
}

int PapaDuck::reconnectWifi(String ssid, String password) {
#ifdef CDPCFG_WIFI_NONE
  Serial.println("[PapaDuck] WARNING reconnectWifi skipped, device has no WiFi.");
  return DUCK_ERR_NONE;
#else

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
#endif
}