#ifndef DUCKWIFINONE_H
#define DUCKWIFINONE_H

#include "../utils/DuckError.h"
#include "../utils/DuckLogger.h"

class DuckWifiNone {
    public:
        int reconnect(std::string ssid, std::string password) {
            logwarn_ln("wifi is disabled");
            return DUCK_ERR_NONE;
        }

        int joinNetwork(std::string ssid, std::string password) {
            logwarn_ln("wifi is disabled");
            return DUCK_ERR_NONE;
        }
        
        bool connected() {
            logwarn_ln("wifi is disabled");
            return false;
        }

        void setSsid(std::string val) {
            logwarn_ln("wifi is disabled");
        }
    
        void setPassword(std::string val) {
            logwarn_ln("wifi is disabled");
        }
    protected:
    private:
        int saveWifiCredentials(std::string ssid, std::string password) {
            logwarn_ln("wifi is disabled");
            return DUCK_ERR_NONE;
        }

        int loadWiFiCredentials() {
            logwarn_ln("wifi is disabled");
            return DUCK_ERR_NONE;
        }
};

#endif