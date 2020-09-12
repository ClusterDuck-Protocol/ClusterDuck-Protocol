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

class DuckNet {
public:
  static DuckNet* getInstance();
  
#ifdef CDPCFG_WIFI_NONE
  void setupWebServer(bool createCaptivePortal = false, String html = "") {}
  void setupWifiAp(const char* accessPoint = " 🆘 DUCK EMERGENCY PORTAL") {}
  void setupDns() {}
  void setupInternet(String ssid, String password) {}
  bool ssidAvailable(String val = "") {return false;}
  void setSsid(String val) {}
  void setPassword(String val) {}
  String getSsid() {return "";}
  String getPassword() {return "";}
  void setDeviceId(String deviceId) {}
#else 
  void setupWebServer(bool createCaptivePortal = false, String html = "");
  void setupWifiAp(const char* accessPoint = " 🆘 DUCK EMERGENCY PORTAL");
  void setupDns();
  void setupInternet(String ssid, String password);
  bool ssidAvailable(String val = "");
  void setSsid(String val);
  void setPassword(String val);
  String getSsid();
  String getPassword();
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