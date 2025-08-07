#ifndef DUCKWIFI_H
#define DUCKWIFI_H

// #ifdef CDPCFG_WIFI_NONE
// #pragma info "WARNING: WiFi is disabled. DuckNet will not be available."
// #else

#include "../utils/DuckError.h"
#include "../utils/DuckLogger.h"
#include "../include/cdpcfg.h"
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

class DuckWifi {
    public:
        /**
         * @brief reconnect the duck to the given wifi access point
         * 
         * @param ssid the access point ssid to connect to 
         * @param password the access point password
         * @return DUCK_ERR_NONE if the duck reconnected to the AP sucessfully. An error code otherwise. 
         */
        int reconnect(std::string ssid, std::string password);

        /**
         * @brief Setup internet access.
         *
         * @param ssid        the ssid of the WiFi network
         * @param password    password to join the network
         */
        int joinNetwork(std::string ssid, std::string password);

        /**
         * @brief Check wifi connection status
         * 
         * @returns true if device wifi is connected, false otherwise. 
         */
        bool connected(); //change to an option with wifi name?

        void setSsid(std::string val);
    
        void setPassword(std::string val);
    protected:
    private:
        /**
         * @brief Save / Write Wifi credentials to EEPROM
         *
         * @param ssid        the ssid of the WiFi network
         * @param password    password to join the network
         * @return DUCK_ERR_NONE if successful, an error code otherwise.
         */
        int saveWifiCredentials(std::string ssid, std::string password);

        /**
         * @brief Load Wifi credentials from EEPROM
         * @return DUCK_ERR_NONE if successful, an error code otherwise.
         */
        int loadWiFiCredentials();
};

#endif