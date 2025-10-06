#include "DuckWifi.h"

int DuckWifi::reconnect(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
}

int DuckWifi::joinNetwork(std::string ssid, std::string password) {
  const uint32_t WIFI_CONNECTION_TIMEOUT_MS = 1500;

  int rc = DUCK_ERR_NONE;
  
  //  Connect to Access Point
  loginfo_ln("setupInternet: connecting to WiFi access point SSID: %s",ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  // We need to wait here for the connection to establish. Otherwise the WiFi.status() may return a false negative
  loginfo_ln("setupInternet: Waiting for connect results for ", ssid.c_str());
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

int saveWifiCredentials(std::string ssid, std::string password) {
    int err = DUCK_ERR_NONE;
  
    if (ssid.empty() || password.empty()) {
      logerr("Invalid SSID or password\n");
      return DUCK_ERR_INVALID_ARGUMENT;
    }
    if (!EEPROM.begin(512)) {
      logerr("Failed to initialise EEPROM\n");
      return DUCK_ERR_EEPROM_INIT;
    }
  
    if (ssid.length() > 0 && password.length() > 0) {
      loginfo("Clearing EEPROM\n");
      for (int i = 0; i < 96; i++) {
        EEPROM.write(i, 0);
      }
  
      loginfo("updating EEPROM...\n");
      for (int i = 0; i < ssid.length(); i++)
      {
        EEPROM.write(i, ssid[i]);
      }
      for (int i = 0; i < password.length(); ++i)
      {
        EEPROM.write(32 + i, password[i]);
      }
      if (!EEPROM.commit()) {
        logerr("Failed to commit EEPROM\n");
        err = DUCK_ERR_EEPROM_WRITE;
      }
    }
    return err;
  }
  
std::string loadWifiSsid() {
    EEPROM.begin(512); //Initialasing EEPROM
    std::string esid;
    // loop through saved SSID characters
    for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
    return esid;
}
  
std::string loadWifiPassword() {
    EEPROM.begin(512); //Initialasing EEPROM
    std::string epass = "";
    // loop through saved Password characters
    for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
    return epass;
}
