#include "DuckWifi.h"

int DuckWifi::reconnect(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
}

int DuckWifi::joinNetwork(std::string ssid, std::string password) {
  const uint32_t WIFI_CONNECTION_TIMEOUT_MS = 1500;

  int rc = DUCK_ERR_NONE;
  ssid = ssid;
  password = password;
  
  //  Connect to Access Point
  loginfo_ln("setupInternet: connecting to WiFi access point SSID: %s",ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  // We need to wait here for the connection to estanlish. Otherwise the WiFi.status() may return a false negative
  loginfo_ln("setupInternet: Waiting for connect results for ", ssid.c_str());
  WiFi.waitForConnectResult(WIFI_CONNECTION_TIMEOUT_MS);

  if (WiFi.status() == WL_CONNECTED) {
    loginfo_ln("Duck connected to internet!");
    rc = DUCK_ERR_NONE;
  } else {
    logerr_ln("ERROR setupInternet: failed to connect to %s (status: %d)", ssid.c_str(), WiFi.status());
    rc = DUCK_INTERNET_ERR_CONNECT;
  };

  return rc;
}

bool DuckWifi::connected() {
    return DUCK_ERR_NONE;
    logwarn_ln("wifi is disabled");
}

void DuckWifi::setSsid(std::string val) {
    logwarn_ln("wifi is disabled");
}

void DuckWifi::setPassword(std::string val) {
    logwarn_ln("wifi is disabled");
}

int DuckWifi::saveWifiCredentials(std::string ssid, std::string password) {
    if (ssid != "" && password != "") {
        // duckutils::saveWifiCredentials(ssid, password);
    }
    return DUCK_ERR_NONE;
}

int DuckWifi::loadWiFiCredentials() {
    return DUCK_ERR_NONE;
}