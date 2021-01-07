#include "include/DuckNet.h"

DuckNet* DuckNet::instance = NULL;

DuckNet::DuckNet() { duckRadio = DuckRadio::getInstance(); }
DuckNet* DuckNet::getInstance() {
  if (instance == NULL) {
    instance = new DuckNet();
  }
  return instance;
}

#ifndef CDPCFG_WIFI_NONE
IPAddress apIP(CDPCFG_AP_IP1, CDPCFG_AP_IP2, CDPCFG_AP_IP3, CDPCFG_AP_IP4);
AsyncWebServer webServer(CDPCFG_WEB_PORT);
DNSServer DuckNet::dnsServer;

const char* DuckNet::DNS = "duck";
const byte DuckNet::DNS_PORT = 53;

// Username and password for /update
const char* http_username = CDPCFG_UPDATE_USERNAME;
const char* http_password = CDPCFG_UPDATE_PASSWORD;

bool restartRequired = false;
size_t content_len;

void DuckNet::setDeviceId(std::vector<byte> deviceId) {
  this->deviceId.insert(this->deviceId.end(), deviceId.begin(), deviceId.end());
}

int DuckNet::setupWebServer(bool createCaptivePortal, String html) {
  loginfo("Setting up Web Server");

  if (txPacket == NULL) {
    txPacket = new DuckPacket(deviceId);
  }

  if (html == "") {
    logdbg("Web Server using main page");
    portal = MAIN_page;
  } else {
    logdbg("Web Server using custom main page");
    portal = html;
  }
  webServer.onNotFound([&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", portal);
  });

  webServer.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", portal);
  });

  // Update Firmware OTA
  webServer.on("/update", HTTP_GET, [&](AsyncWebServerRequest* request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    AsyncWebServerResponse* response =
        request->beginResponse(200, "text/html", update_page);

    request->send(response);
  });

  webServer.on(
      "/update", HTTP_POST,
      [&](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(
            (Update.hasError()) ? 500 : 200, "text/plain",
            (Update.hasError()) ? "FAIL" : "OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        restartRequired = true;
      },
      [&](AsyncWebServerRequest* request, String filename, size_t index,
          uint8_t* data, size_t len, bool final) {
        if (!index) {

          loginfo("Pause Radio and starting OTA update");
          duckRadio->standBy();
          content_len = request->contentLength();

          int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {

            Update.printError(Serial);
          }
        }

        if (Update.write(data, len) != len) {
          Update.printError(Serial);
          duckRadio->startReceive();
        }

        if (final) {
          if (Update.end(true)) {
            ESP.restart();
            esp_task_wdt_init(1, true);
            esp_task_wdt_add(NULL);
            while (true)
              ;
          }
        }
      });

  // Captive Portal form submission
  webServer.on("/formSubmit", HTTP_POST, [&](AsyncWebServerRequest* request) {
    loginfo("Submitting Form");

    int err = DUCK_ERR_NONE;

    int paramsNumber = request->params();
    String val = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);
      logdbg(p->name() + ": " + p->value());

      val = val + p->value().c_str() + "*";
    }

    std::vector<byte> data;
    data.insert(data.end(), val.begin(), val.end());
    //TODO: send the correct ducktype
    txPacket->prepareForSending(ZERO_DUID, DuckType::UNKNOWN, topics::status, data );
    err = duckRadio->sendData(txPacket->getBuffer());

    switch (err) {
      case DUCK_ERR_NONE:
        request->send(200, "text/html", portal);
        break;
      case DUCKLORA_ERR_MSG_TOO_LARGE:
        request->send(413, "text/html", "Message payload too big!");
        break;
      case DUCKLORA_ERR_HANDLE_PACKET:
        request->send(400, "text/html", "BadRequest");
        break;
      default:
        request->send(500, "text/html", "Oops! Unknown error.");
        break;
    }
  });

  webServer.on("/id", HTTP_GET, [&](AsyncWebServerRequest* request) {
    std::string id(deviceId.begin(), deviceId.end());
    request->send(200, "text/html", id.c_str());
  });

  webServer.on("/restart", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Restarting...");
    delay(1000);
    duckesp::restartDuck();
  });

  webServer.on("/mac", HTTP_GET, [&](AsyncWebServerRequest* request) {
    String mac = duckesp::getDuckMacAddress(true);
    request->send(200, "text/html", mac);
  });

  webServer.on("/wifi", HTTP_GET, [&](AsyncWebServerRequest* request) {
     request->send(200, "text/html", wifi_page);
    
  });

  webServer.on("/changeSSID", HTTP_POST, [&](AsyncWebServerRequest* request) {
    int paramsNumber = request->params();
    String val = "";
    String ssid = "";
    String password = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);

      String name = String(p->name());
      String value = String(p->value());

      if (name == "ssid") {
        ssid = String(p->value());
      } else if (name == "pass") {
        password = String(p->value());
      }
    }

    if (ssid != "" && password != "") {
      setupInternet(ssid, password);
      request->send(200, "text/plain", "Success");
    } else {
      request->send(500, "text/plain", "There was an error");
    }
  });

  webServer.begin();

  return DUCK_ERR_NONE;
}

int DuckNet::setupWifiAp(const char* accessPoint) {

  bool success;

  success = WiFi.mode(WIFI_AP);
  if (!success) {
    return DUCKWIFI_ERR_AP_CONFIG;
  }

  success = WiFi.softAP(accessPoint);
  if (!success) {
    return DUCKWIFI_ERR_AP_CONFIG;
  }
  //TODO: need to find out why there is a delay here
  delay(200);
  success = WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (!success) {
    return DUCKWIFI_ERR_AP_CONFIG;
  }

  loginfo("Created Wifi Access Point");
  return DUCK_ERR_NONE;
}

int DuckNet::setupDns() {
  bool success = dnsServer.start(DNS_PORT, "*", apIP);

  if (!success) {
    logerr("ERROR dns server start failed");
    return DUCKDNS_ERR_STARTING;
  }

  success = MDNS.begin(DNS);
 
  if (!success) {
    logerr("ERROR dns server begin failed");
    return DUCKDNS_ERR_STARTING;
  }

  loginfo("Created local DNS");
  MDNS.addService("http", "tcp", CDPCFG_WEB_PORT);

  return DUCK_ERR_NONE;
}

int DuckNet::setupInternet(String ssid, String password) {
  this->ssid = ssid;
  this->password = password;
  if (ssid == "" || password == "") {
    logerr("ERROR setupInternet: Please provide an ssid and password for connecting to a wifi access point");
    return DUCK_INTERNET_ERR_SETUP;
  }
  if (!ssidAvailable(ssid)) {
    logerr("ERROR setupInternet: " + ssid + " is not available. Please check the provided ssid and/or passwords");
    return DUCK_INTERNET_ERR_SSID;
  }
  // Connect to Access Point
  WiFi.begin(ssid.c_str(), password.c_str());

  // TODO: we should probably simply fail here and let the app decide what to do
  // Continuous retry could deplete the battery.
  // Also this will only work if we have a reboot timer handler implemented
  // otherwise we are just going to be stuck in this loop
  while (WiFi.status() != WL_CONNECTED) {
    logerr("ERROR setupInternet: failed to connect to " + ssid);
    duckutils::getTimer().tick(); // Advance timer to reboot after awhile
  }

  loginfo("Duck connected to internet!");

  return DUCK_ERR_NONE;
}

bool DuckNet::ssidAvailable(String val) {

  int n = WiFi.scanNetworks();
  logdbg("scan done. ");
  if (n == 0 || ssid == "") {
    logdbg("Networks found: "+String(n));
  } else {
    logdbg("Networks found: "+String(n));
    if (val == "") {
      val = ssid;
    }
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) == val) {
        logdbg("Given ssid is available!");
        return true;
      }
      delay(AP_SCAN_INTERVAL_MS);
    }
  }
  loginfo("No ssid available");

  return false;
}

void DuckNet::setSsid(String val) { ssid = val; }

void DuckNet::setPassword(String val) { password = val; }

String DuckNet::getSsid() { return ssid; }

String DuckNet::getPassword() { return password; }

#endif
