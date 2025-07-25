#include "DuckWifi.h"

int DuckWifi::reconnect(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
}

int DuckWifi::setupAccessPoint(const char* ap = "🆘 DUCK EMERGENCY PORTAL") {
    bool success;

    success = WiFi.mode(WIFI_AP);
    if (!success) {
      return DUCKWIFI_ERR_AP_CONFIG;
    }
  
    success = WiFi.softAP(ap);
    if (!success) {
      return DUCKWIFI_ERR_AP_CONFIG;
    }
    //TODO: need to find out why there is a delay here
    delay(200);
    success = WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    if (!success) {
      return DUCKWIFI_ERR_AP_CONFIG;
    }
  
    loginfo_ln("Created Wifi Access Point");
    return DUCK_ERR_NONE;
}

// int DuckWifi::setupDns() {
//     bool success = dnsServer.start(DNS_PORT, "*", apIP);

//     if (!success) {
//         logerr_ln("ERROR dns server start failed");
//         return DUCKDNS_ERR_STARTING;
//     }

//     success = MDNS.begin(DNS);
    
//     if (!success) {
//         logerr_ln("ERROR dns server begin failed");
//         return DUCKDNS_ERR_STARTING;
//     }

//     loginfo_ln("Created local DNS");
//     MDNS.addService("http", "tcp", CDPCFG_WEB_PORT);

//     return DUCK_ERR_NONE;
// }

int DuckWifi::joinNetwork(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
    logwarn_ln("wifi is disabled");
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
    return DUCK_ERR_NONE;
}

int DuckWifi::loadWiFiCredentials() {
    return DUCK_ERR_NONE;
}