#include "DuckWifi.h"

int DuckWifi::reconnect(std::string ssid, std::string password) {
    return DUCK_ERR_NONE;
}

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