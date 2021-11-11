/**
 * @file DuckNet.h
 * @brief This file is internal to CDP and provides the library access to
 * networking functions.
 *
 * The implementation is conditioned by the `CDPCFG_WIFI_NONE` flag which may be
 * defined in `cdpcfh.h` to disable WiFi.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */

#pragma once

#include <WString.h>

#include "cdpcfg.h"

class DuckNet;
// Since Duck needs to know about DuckNet and DuckNet needs to know about Duck,
// this forward declaration allows a DuckNet reference to be declared in Duck.h.

#include "Duck.h"

#ifdef CDPCFG_WIFI_NONE

#include "DuckUtils.h"

#else

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "../DuckError.h"
#include "DuckEsp.h"
#include "DuckUtils.h"
#include "OTAPage.h"
#include "index.h"
#include "wifiCredentials.h"
#include "controlPanel.h"
#include "cdpHome.h"
#include "papaHome.h"

#endif

#define AP_SCAN_INTERVAL_MS 10

/**
 * @brief Internal network abstraction.
 *
 * Provides access to Webserver, DNS, WiFi and OTA update functionalities.
 */
class DuckNet {
public:

  int channel;

#ifdef CDPCFG_WIFI_NONE
  int setupWebServer(bool createCaptivePortal = false, String html = "") {
    logwarn("WARNING setupWebServer skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }
  int setupWifiAp(const char* accessPoint = "DuckLink") {
    logwarn("WARNING setupWifiAp skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }
  int setupDns() {
    logwarn("WARNING setupDns skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }

  int setupInternet(String ssid, String password) {
    logwarn("WARNING setupInternet skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }
  
  bool ssidAvailable(String val = "") { return false; }
  void setSsid(String val) {}
  void setPassword(String val) {}
  String getSsid() { return ""; }
  String getPassword() { return ""; }
  // int getChannel();
  void setDeviceId(std::vector<byte> deviceId) {}
  bool isWifiConnected() { return false; }
  int loadWiFiCredentials(){return DUCK_ERR_NONE; };
#else
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
  int setupWebServer(bool createCaptivePortal = false, String html = "");

  /**
   * @brief Set up the WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to  "DuckLink"
   *
   * @returns DUCK_ERR_NONE if successful, an error code otherwise.
   */
  int setupWifiAp(const char* accessPoint = "DuckLink");

  /**
   * @brief Set up DNS.
   *
   * @returns DUCK_ERR_NONE if sucessful, an error code otherwise
   */
  int setupDns();

  /**
   * @brief Set up internet access.
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   */
  int setupInternet(String ssid, String password);

  /**
   * @brief  Checks if the given ssid is available.
   *
   * @param val     ssid to check, default is an empty string and will use the
   * internal default ssid
   * @returns true if the ssid is available, false otherwise.
   */
  bool ssidAvailable(String val = "");

  /**
   * @brief Save / Write Wifi credentials to EEPROM
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   * @return DUCK_ERR_NONE if successful, an error code otherwise.
   */
  int saveWifiCredentials(String ssid, String password);
  
  /**
   * @brief save set radio channel
   *
   * @param val channel number to be set
   */
  void saveChannel(int val);
  
  /**
   * @brief Load channel number saved in eeprom
   */
  void loadChannel();
  
  /**
   * @brief Load Wifi credentials from EEPROM
   * @return DUCK_ERR_NONE if successful, an error code otherwise.
   */
  int loadWiFiCredentials();
  
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
   * @returns a string representing the current network ssid
   */
  String getSsid();

  /**
   * @brief Get the WiFi password ssid.
   *
   * @returns a string representing the current network password
   */
  String getPassword();

  /**
   * @brief Get the current channel.
   *
   * @returns an int representing the current channel
   */
  int getChannel();

  /**
   * @brief Set the Duck's device id.
   *
   * @param deviceId Duck's device ID string to set
   */
  void setDeviceId(std::vector<byte> deviceId);

  /**
   * @brief Provide Wifi connection status.
   *
   * @returns true if wifi is connected, false otherwise.
   */
  bool isWifiConnected() { return (WiFi.status() == WL_CONNECTED); }

  static DNSServer dnsServer;
#endif

  DuckNet(Duck* duck);

private:

  String getMuidStatusMessage(muidStatus status);
  String getMuidStatusString(muidStatus status);
  String createMuidResponseJson(muidStatus status);

  DuckNet(DuckNet const&) = delete;
  DuckNet& operator=(DuckNet const&) = delete;

  Duck* duck;
  std::vector<byte> deviceId;

  static const byte DNS_PORT;
  static const char* DNS;
  static const char* AP;
  String portal = "";
  String ssid = "";
  String password = "";
  // char* controlSsid = "";
  // char* controlPassword = "";
};
