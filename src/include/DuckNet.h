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

// Since Duck needs to know about DuckNet and DuckNet needs to know about Duck,
// this forward declaration allows a DuckNet reference to be declared in Duck.h.

#include <map>
#include "DuckUtils.h"
#include "DuckError.h"
#include "DuckLogger.h"
#include <string>

#include "DuckRadio.h"
#include "bloomfilter.h"
#include "DuckPacket.h"

#ifdef CDPCFG_WIFI_NONE
#pragma info "WARNING: WiFi is disabled. DuckNet will not be available."
#else

#include <Update.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "DuckEsp.h"
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
 * Provides access to Webserver, DNS, and WiFi update functionalities.
 */
class DuckNet {
public:

  int channel;
  std::string duckSession;

#ifdef CDPCFG_WIFI_NONE
  int setupWebServer(bool createCaptivePortal = false, std::string html = "") {
    logwarn_ln("WARNING setupWebServer skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }

  int setupWifiAp(const char* accessPoint = "DuckLink") {
    logwarn_ln("WARNING setupWifiAp skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }

  int setupDns() {
    logwarn_ln("WARNING setupDns skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }

  int setupInternet(std::string ssid, std::string password) {
    logwarn_ln("WARNING setupInternet skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }

  void setSsid(std::string val) {
    logwarn_ln("WARNING setSsid skipped, device has no WiFi.");
  }

  void setPassword(std::string val) {
    logwarn_ln("WARNING setPassword skipped, device has no WiFi.");
  }

  std::string getSsid() {
    logwarn_ln("WARNING getSsid skipped, device has no WiFi.");
    return std::string("");
  }

  std::string getPassword() {
    logwarn_ln("WARNING getPassword skipped, device has no WiFi.");
    return std::string("");
  }

  void setDeviceId(std::vector<byte> deviceId) {
    logwarn_ln("WARNING setDeviceId skipped, device has no WiFi.");
  }

  bool isWifiConnected() {
    logwarn_ln("WARNING isWifiConnected skipped, device has no WiFi.");
    return false;
  }

  int loadWiFiCredentials() {
    logwarn_ln("WARNING loadWiFiCredentials skipped, device has no WiFi.");
    return DUCK_ERR_NONE;
  }

  void saveChannel(int val) {
    logwarn_ln("WARNING saveChannel skipped, device has no WiFi.");
  }

  int getChannel() {
    logwarn_ln("WARNING getChannel skipped, device has no WiFi.");
    return 0;
  }

  void loadChannel() {
    logwarn_ln("WARNING loadChannel skipped, device has no WiFi.");
  }

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
  int setupWebServer(bool createCaptivePortal = false, std::string html = "");

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
  int setupInternet(std::string ssid, std::string password);

  /**
   * @brief Save / Write Wifi credentials to EEPROM
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   * @return DUCK_ERR_NONE if successful, an error code otherwise.
   */
  int saveWifiCredentials(std::string ssid, std::string password);

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
  void setSsid(std::string val);

  /**
   * @brief Set the WiFi password.
   *
   * @param val  the password string to set
   */
  void setPassword(std::string val);

  /**
   * @brief Get the WiFi network ssid.
   *
   * @returns a string representing the current network ssid
   */
  std::string getSsid();

  /**
   * @brief Get the WiFi password ssid.
   *
   * @returns a string representing the current network password
   */
  std::string getPassword();

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
  void setDeviceId(std::array<byte,8> deviceId);

  /**
   * @brief Provide Wifi connection status.
   *
   * @returns true if wifi is connected, false otherwise.
   */
  bool isWifiConnected() { return (WiFi.status() == WL_CONNECTED); }

  static DNSServer dnsServer;
#endif

  DuckNet(BloomFilter *bloomFilter);

private:

  DuckNet(DuckNet const&) = delete;
  DuckNet& operator=(DuckNet const&) = delete;

  DuckRadio& duckRadio = DuckRadio::getInstance();

  DuckPacket* txPacket = NULL;

  std::array<byte,8> deviceId;

  BloomFilter *bloomFilter = nullptr;

  static const byte DNS_PORT;
  static const char* DNS;
  static const char* AP;
  std::string portal = "";
  std::string ssid = "";
  std::string password = "";
  // char* controlSsid = "";
  // char* controlPassword = "";
};
