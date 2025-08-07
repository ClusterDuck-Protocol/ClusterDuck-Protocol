/**
 * @file CaptivePortal.h
 * @brief This file is internal to CDP and provides the web server functionality of the default Captive Portal.
 *
 * The implementation is conditioned by the `CDPCFG_WIFI_NONE` flag which may be
 * defined in `cdpcfh.h` to disable WiFi.
 * @version
 * @date 2025-08-6
 *
 * @copyright
 */

#pragma once

#include "../utils/DuckUtils.h"
#include "../utils/DuckError.h"
#include "../utils/DuckLogger.h"

// #include "../radio/DuckLoRa.h"
#include "../DuckPacket.h"

#ifdef CDPCFG_WIFI_NONE
#pragma info "WARNING: WiFi is disabled. CaptivePortal will not be available."
#else

#include <Update.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

#include "../DuckEsp.h"
#include "../CaptivePortal/portalPages/index.h"
#include "../CaptivePortal/portalPages/wifiCredentials.h"
#include "../CaptivePortal/portalPages/controlPanel.h"
#include "../CaptivePortal/portalPages/cdpHome.h"
#include "../CaptivePortal/portalPages/papaHome.h"


#endif

#define AP_SCAN_INTERVAL_MS 10
class CaptivePortal {
public:

  int channel;
  std::string duckSession;

  #ifdef CDPCFG_WIFI_NONE
  void setupAccessPoint(const char* ap = "🆘 DUCK EMERGENCY PORTAL"){
    logwarn_ln("WARNING setSsid skipped, device has no WiFi.");
  }
  void setupDns(){
    logwarn_ln("WARNING setSsid skipped, device has no WiFi.");
  }
  void setSsid(std::string val) {
    logwarn_ln("WARNING setSsid skipped, device has no WiFi.");
  }

  void setPassword(std::string val) {
    logwarn_ln("WARNING setPassword skipped, device has no WiFi.");
  }
  #else

    /**
     * @brief Setup WiFi access point.
     *
     * @param accessPoint a string representing the access point. Default to
     * "🆘 DUCK EMERGENCY PORTAL"
     *
     * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
     */
    int setupAccessPoint(const char* ap = "🆘 DUCK EMERGENCY PORTAL");

    /**
     * @brief Setup DNS for Captive Portal
     *
     * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
     */
    int setupDns();
      /**
       * @brief Set up the WebServer.
       *
       * The WebServer is used to communicate with the Duck over ad-hoc WiFi
       * connection.
       *
       * @param createCaptivePortal set to true if Captive WiFi connection is
       * needed. Defaults to false
       * @param html A string representing custom HTML code used for the portal.
       * Default is an empty string Default portal web page is used if the string is
       * empty
       */
      int setupWebServer();

  #endif

    CaptivePortal(int port): port(port), webServer(port), events("/events") {}

  private:

    CaptivePortal(CaptivePortal const&) = delete;
    CaptivePortal& operator=(CaptivePortal const&) = delete;
    IPAddress apIP{CDPCFG_AP_IP1, CDPCFG_AP_IP2, CDPCFG_AP_IP3, CDPCFG_AP_IP4};
    DNSServer dnsServer;
    const char* DNS = "duck";
    const byte DNS_PORT = 53;

    //must be initialized in this order for webserver to work
    int port;
    AsyncWebServer webServer;
    AsyncEventSource events;

    DuckPacket* txPacket = NULL;

    std::array<byte,8> deviceId;

    std::string portal = "";
    std::string ssid = "";
    std::string password = "";
};
