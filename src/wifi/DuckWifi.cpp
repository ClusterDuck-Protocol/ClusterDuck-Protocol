#include "DuckWifi.h"

int DuckWifi::reconnect(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
}

int DuckWifi::joinNetwork(std::string ssid, std::string password) {
  const uint32_t WIFI_CONNECTION_TIMEOUT_MS = 1500;

  int rc = DUCK_ERR_NONE;
  
  //  Connect to Access Point
  loginfo_ln("setupInternet: connecting to WiFi access point SSID: %s", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  // We need to wait here for the connection to establish. Otherwise the WiFi.status() may return a false negative
  loginfo_ln("setupInternet: Waiting for connect results for %s", ssid.c_str());
  WiFi.waitForConnectResult(WIFI_CONNECTION_TIMEOUT_MS);

  if (connected()) {
    loginfo_ln("Duck connected to internet!");
    rc = DUCK_ERR_NONE;
  } else {
    logerr_ln("ERROR setupInternet: failed to connect to %s (status: %d)", ssid.c_str(), WiFi.status());
    rc = DUCK_INTERNET_ERR_CONNECT;
  };

  return rc;
}

bool DuckWifi::connected() {
    return (WiFi.status() == WL_CONNECTED);
}

int DuckWifi::saveWifiCredentials(std::string ssid, std::string password) {
    int err = DUCK_ERR_NONE;
  
    if (ssid.empty() || password.empty()) {
      logerr_ln("Invalid SSID or password\n");
      return DUCK_ERR_INVALID_ARGUMENT;
    }
    String ssidStr(ssid.c_str());
    String passStr(password.c_str());

    bool wifiSaved = wifi_eeprom.putString("wifi_ssid", ssidStr);
    bool passSaved = wifi_eeprom.putString("wifi_pass", passStr);

    if (!wifiSaved || !passSaved) {
        logerr_ln("Failed to save WiFi credentials");
        return DUCK_ERR_EEPROM_WRITE;
    }
    loginfo_ln("WiFi credentials saved successfully");
    wifi_eeprom.end();
    return err;
  }

std::optional<std::string> DuckWifi::loadWifiSsid() {
  if(initCredentialStorage()){
    String pass = wifi_eeprom.getString("wifi_pass", "");
    wifi_eeprom.end();
    return std::string(pass.c_str());
  } else{
    logerr_ln("failed to load credentials -- eeprom preferences error");
    return std::nullopt;
  }
}
  

std::optional<std::string> DuckWifi::loadWifiPassword() {
  if(initCredentialStorage()){
    String ssid = wifi_eeprom.getString("wifi_ssid", "");
    wifi_eeprom.end();
    return std::string(ssid.c_str());
  } else{
    logerr_ln("failed to load credentials -- eeprom preferences error");
    return std::nullopt;
  }
}

bool DuckWifi::initCredentialStorage(){
  if(!this->eeprom_initialized){
    if (!wifi_eeprom.begin("duckwifi")) {
      logerr_ln("Failed to initialise wifi preferences storage");
      this->eeprom_initialized = false;
      return false;
    } else{
      this->eeprom_initialized = true;
      return true;
    }
  } else {
    return true;
  }
}