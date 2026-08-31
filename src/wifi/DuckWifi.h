#ifndef DUCKWIFI_H
#define DUCKWIFI_H

#ifdef CDPCFG_WIFI_NONE
 #pragma info "WARNING: WiFi is disabled. DuckNet will not be available."
#else

#include "../utils/DuckError.h"
#include "../utils/DuckLogger.h"
#include "../include/cdpcfg.h"
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

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
        
        /**
         * @brief Write ssid to EEPROM
         *
         * @param val        the new ssid of the WiFi network
         */
        void setSsid(std::string val);

        /**
         * @brief Write password to EEPROM
         *
         * @param val        the new password of the WiFi network
         */
        void setPassword(std::string val);

        /**
         * @brief retrieve ssid preference (previously eeprom)
         * @return an optional with a string with the local storage wifi network ssid if retrieval success otherwise nullopt
         */
        std::optional<std::string> loadWifiSsid();

        /**
         * @brief retrieve password preference (previously eeprom)
         * @return an optional with a string with the local storage wifi network password if retrieval success otherwise nullopt
         */
        std::optional<std::string> loadWifiPassword();
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
         * @brief initialize preferences for credentials storage (previously eeprom)
         * @returns true if successful false if failure
         */
        bool initCredentialStorage();
        bool eeprom_initialized = false;
        Preferences wifi_eeprom;
};
#endif
#endif