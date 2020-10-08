#include "../PapaDuck.h"

void PapaDuck::setupWithDefaults(String ssid, String password) {
  Duck::setupWithDefaults(ssid, password);
  setupRadio();
  
  if (!ssid.isEmpty() && !password.isEmpty()) {
    setupWifi("PapaDuck Setup");
    setupDns();
    setupInternet(ssid, password);
    setupWebServer(false);
    setupOTA();
  }
  Serial.println("PapaDuck setup done");
}

void PapaDuck::handleReceivedPacket() {

  Serial.println("[PapaDuck] handleReceivedPacket()...");

  rxPacket->reset();

  std::vector<byte> data;
  int err = duckLora->getReceivedData(&data);

  if (err != DUCK_ERR_NONE) {
    Serial.print("[PapaDuck] ERROR - failed to get data from DuckLora. rc = ");
    Serial.println(err);
    return;
  }

  bool relay = rxPacket->update(duid, data);
  if (relay) {
    Serial.println("[PapaDuck] relaying packet " +
                   rxPacket->getPathAsHexString());
    recvDataCallback(rxPacket->getCdpPacket());
  }
}
void PapaDuck::run() {

  handleOtaUpdate();
  if (getReceiveFlag()) {
    setReceiveFlag(false);
    duckutils::setDuckInterrupt(false);
    
    handleReceivedPacket();

    duckutils::setDuckInterrupt(true);
    startReceive();
  }
}

int PapaDuck::reconnectWifi(String ssid, String password) {
#ifdef CDPCFG_WIFI_NONE
  return DUCK_ERR_NOT_SUPPORTED;
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