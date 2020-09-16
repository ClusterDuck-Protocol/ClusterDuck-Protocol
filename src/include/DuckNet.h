#ifndef DUCKNET_H_
#define DUCKNET_H_

#include <WString.h>
#include "cdpcfg.h"

#ifdef CDPCFG_WIFI_NONE

#include "DuckLora.h"
#include "DuckUtils.h"

#else
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include "DuckEsp.h"
#include "DuckLora.h"
#include "DuckUtils.h"
#include "OTAPage.h"
#include "index.h"

#endif

#define AP_SCAN_INTERVAL_MS 10
/**
 * @brief Network abstraction.
 * 
 * Provides access to Webserver, DNS, WiFi and OTA update functionalities.
 */
class DuckNet {
public:
  /**
   * @brief Get a singletom instance of the DuckNet class.
   *
   * @return A pointer to DuckNet object.
   */
  static DuckNet* getInstance();
  
#ifdef CDPCFG_WIFI_NONE
  void setupWebServer(bool createCaptivePortal = false, String html = "") {}
  void setupWifiAp(const char* accessPoint = "🆘 DUCK EMERGENCY PORTAL") {}
  void setupDns() {}
  void setupInternet(String ssid, String password) {}
  bool ssidAvailable(String val = "") {return false;}
  void setSsid(String val) {}
  void setPassword(String val) {}
  String getSsid() {return "";}
  String getPassword() {return "";}
  void setDeviceId(String deviceId) {}
#else 
  /**
   * @brief Set up the WebServer.
   * 
   * The WebServer is used to communicate with the Duck over ad-hoc WiFi connection.
   * 
   * @param createCaptivePortal set to true if Captive WiFi connection is needed. Defaults to false
   * @param html A string representing custom HTML code used for the portal. Default is an empty string
   * Default portal web page is used if the string is empty
   */
  void setupWebServer(bool createCaptivePortal = false, String html = "");

  /**
   * @brief Set up the WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to  "🆘 DUCK EMERGENCY PORTAL"
   */
  void setupWifiAp(const char* accessPoint = "🆘 DUCK EMERGENCY PORTAL");
  /**
   * @brief Set up DNS.
   * 
   */
  void setupDns();
  /**
   * @brief Set up internet access.
   * 
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network 
   */
  void setupInternet(String ssid, String password);

  /**
   * @brief  Checks if the given ssid is available.
   * 
   * @param val     ssid to check, default is an empty string and will use the internal default ssid
   * @return true if the ssid is available, false otherwise.
   */
  bool ssidAvailable(String val = "");

  /**
   * @brief Set the WiFi network ssid.
   * 
   * @param val the ssid string to set
   */
  void setSsid(String val);

  /**
   * @brief Set the WiFi password.
   * 
   * @param val  the password string to set
   */
  void setPassword(String val);

  /**
   * @brief Get the WiFi network ssid.
   * 
   * @return a string representing the current network ssid
   */
  String getSsid();

  /**
   * @brief Get the WiFi password ssid.
   *
   * @return a string representing the current network password
   */
  String getPassword();

  /**
   * @brief Set the Duck's device id.
   * 
   * @param deviceId Duck's device ID string to set
   */
  void setDeviceId(String deviceId);

  static DNSServer dnsServer;
#endif

private:
  DuckNet();
  DuckNet(DuckNet const&) = delete;
  DuckNet& operator=(DuckNet const&) = delete;
  static DuckNet* instance;

  DuckLora* _duckLora;
  String _deviceId;

  static const byte DNS_PORT;
  static const char* DNS;
  static const char* AP;
  String portal = "";
  String ssid = "";
  String password = "";
};


#endif