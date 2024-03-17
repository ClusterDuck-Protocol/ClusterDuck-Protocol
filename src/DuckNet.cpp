#include "include/DuckNet.h"

DuckNet::DuckNet(Duck* duckIn): duck(duckIn) {
}

#ifndef CDPCFG_WIFI_NONE

IPAddress apIP(CDPCFG_AP_IP1, CDPCFG_AP_IP2, CDPCFG_AP_IP3, CDPCFG_AP_IP4);
AsyncWebServer webServer(CDPCFG_WEB_PORT);
AsyncEventSource events("/events");

DNSServer DuckNet::dnsServer;

const char* DuckNet::DNS = "duck";
const byte DuckNet::DNS_PORT = 53;

// Username and password for /update
const char* update_username = CDPCFG_UPDATE_USERNAME;
const char* update_password = CDPCFG_UPDATE_PASSWORD;

const char* control_username = CDPCFG_UPDATE_USERNAME;
const char* control_password = CDPCFG_UPDATE_PASSWORD;

bool restartRequired = false;

void DuckNet::setDeviceId(std::vector<byte> deviceId) {
  this->deviceId.insert(this->deviceId.end(), deviceId.begin(), deviceId.end());
}

std::string DuckNet::getMuidStatusMessage(muidStatus status) {
  switch (status) {
  case invalid:
    return "Invalid MUID.";
  case unrecognized:
    return "Unrecognized MUID. Please try again.";
  case not_acked:
    return "Message sent, waiting for server to acknowledge.";
  case acked:
    return "Message acknowledged.";
  default:
    logerr_ln("Unrecognized muidStatus");
    return "Unrecognized muidStatus";
  }
}

std::string DuckNet::getMuidStatusString(muidStatus status) {
  switch (status) {
  case invalid:
    return "invalid";
  case unrecognized:
    return "unrecognized";
  case not_acked:
    return "not_acked";
  case acked:
    return "acked";
  default:
    return "error";
  }
}

std::string DuckNet::createMuidResponseJson(muidStatus status) {
  std::string statusStr = getMuidStatusString(status);
  std::string message = getMuidStatusMessage(status);
  return "{\"status\":\"" + statusStr + "\", \"message\":\"" + message + "\"}";
}

int DuckNet::setupWebServer(bool createCaptivePortal, std::string html) {
  loginfo_ln("Setting up Web Server");
  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  //HTTP Basic authentication
  webServer.addHandler(&events);

  if (html == "") {
    logdbg_ln("Web Server using main page");
    portal = home_page;
  } else {
    logdbg_ln("Web Server using custom main page");
    portal = html;
  }
  webServer.onNotFound([&](AsyncWebServerRequest* request) {
    logwarn_ln("DuckNet - onNotFound: %s", request->url().c_str());
    request->send(200, "text/html", portal.c_str());
  });

  webServer.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", portal.c_str());
  });

  webServer.on("/main", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", MAIN_page);
  });

  webServer.on("/papamain", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", papa_page);
  });
  
  // This will serve as an easy to access "control panel" to change settings of devices easily
  // TODO: Need to be able to turn off this feature from the application layer for security
  // TODO: Can we limit controls depending on the duck?
  webServer.on("/controlpanel", HTTP_GET, [&](AsyncWebServerRequest* request) {
    // if(controlSsid == "" || controlPassword == "") {
    //   int empty = loadControlCredentials();
    //   Serial.println("Empty: " + empty);
    //   if(empty) {
    //     Serial.println(control_username);
    //     Serial.println(control_password);
    //     if (!request->authenticate(control_username, control_password))
    //   return request->requestAuthentication();
    //   } else {
    //     Serial.println(controlSsid);
    //     Serial.println(controlPassword);
    //     if (!request->authenticate(controlSsid, controlPassword))
    //   return request->requestAuthentication();
    //   }

    // } else {
    //   Serial.println('ELSE');
    //   Serial.println(controlSsid);
    //   Serial.println(controlPassword);
    //   if (!request->authenticate(controlSsid, controlPassword))
    //   return request->requestAuthentication();
    // }
    
    AsyncWebServerResponse* response =
    request->beginResponse(200, "text/html", controlPanel);

    request->send(response);
    
  });

  webServer.on("/flipDetector", HTTP_GET, [&](AsyncWebServerRequest* request) {
    //Run flip method
    duckutils::flipDetectState();
    request->send(200, "text/plain", "Success");
  });

  webServer.on("/flipDecrypt", HTTP_GET, [&](AsyncWebServerRequest* request) {
    //Flip Decrypt State
    loginfo_ln("Flipping Decrypt");

    duck->setDecrypt(!duck->getDecrypt());
    loginfo_ln("Decrypt is now: %d", duck->getDecrypt());
    request->send(200, "text/plain", "Success");
  });

  webServer.on("/setChannel", HTTP_POST, [&](AsyncWebServerRequest* request) {
    AsyncWebParameter* p = request->getParam(0);
    logdbg_ln("%s : %d", p->name(), p->value());
    int val = std::atoi(p->value().c_str());
    duck->setChannel(val);
    saveChannel(val);

    request->send(200, "text/plain", "Success");
  });

  // webServer.on("/changeControlPassword", HTTP_POST, [&](AsyncWebServerRequest* request) {
  //   int paramsNumber = request->params();
  //   std::string val = "";
  //   std::string ssid = "";
  //   std::string password = "";
  //   std::string newSsid = "";
  //   std::string newPassword = "";

  //   for (int i = 0; i < paramsNumber; i++) {
  //     AsyncWebParameter* p = request->getParam(i);

  //     std::string name = String(p->name());
  //     std::string value = String(p->value());

  //     if (name == "ssid") {
  //       ssid = String(p->value());
  //     } else if (name == "pass") {
  //       password = String(p->value());
  //     } else if (name == "newSsid") {
  //       newSsid = String(p->value());
  //     } else if (name == "newPassword") {
  //       newPassword = String(p->value());
  //     }
  //   }

  //   if (ssid == controlSsid && password == controlPassword && newSsid != "" && newPassword != "") {
  //     saveControlCredentials(newSsid, newPassword);
  //     request->send(200, "text/plain", "Success");
  //   } else {
  //     request->send(500, "text/plain", "There was an error");
  //   }
  // });

  // Update Firmware OTA
  webServer.on("/update", HTTP_GET, [&](AsyncWebServerRequest* request) {
    if (!request->authenticate(update_username, update_password))
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
      uint8_t* data, size_t len, bool final)
    {
      duck->updateFirmware(filename.c_str(), index, data, len, final);
      // TODO: error/exception handling
      // TODO: request->send
    });

  webServer.on("/muidStatus.json", HTTP_GET, [&](AsyncWebServerRequest* request) {
    logdbg_ln("muidStatus.json: %s", request->url().c_str());
    std::string muid;
    int paramsNumber = request->params();
    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);
      logdbg_ln("param(%d) %s : %s", i, p->name(), p->value().c_str());
      if (p->name() == "muid") {
        muid = p->value().c_str();
      }
    }

    if (muid.length() != 4) {
      request->send(400, "text/json", createMuidResponseJson(invalid).c_str());
      return;
    }

    std::vector<byte> muidVect = {static_cast<unsigned char>(muid[0]), 
                                  static_cast<unsigned char>(muid[1]), 
                                  static_cast<unsigned char>(muid[2]), 
                                  static_cast<unsigned char>(muid[3])};
                                  
    muidStatus status = duck->getMuidStatus(muidVect);

    std::string jsonResponse = createMuidResponseJson(status);
    switch (status) {
    case invalid:
      request->send(400, "text/json", jsonResponse.c_str());
      break;
    case unrecognized:
    case not_acked:
    case acked:
      request->send(200, "text/json", jsonResponse.c_str());
      break;
    }
  });

  // Captive Portal form submission
  webServer.on("/formSubmit.json", HTTP_POST, [&](AsyncWebServerRequest* request) {
    loginfo_ln("Submitting Form to /formSubmit.json");

    int err = DUCK_ERR_NONE;

    int paramsNumber = request->params();
    std::string val = "";
    std::string clientId = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);
      logdbg_ln("%s : %s", p->name(), p->value());

      if (p->name() == "clientId") {
        clientId = p->value().c_str();
      } else {
        val = val + p->value().c_str() + "*";
      }
    }
    clientId = duckutils::toUpperCase(clientId); 
    val = "[" + clientId + "]" + val;
    std::vector<byte> muid;
    err = duck->sendData(topics::cpm, val, ZERO_DUID, &muid);

    switch (err) {
      case DUCK_ERR_NONE:
      {
        std::string response = "{\"muid\":\"" + duckutils::toString(muid) + "\"}";
        request->send(200, "text/html", response.c_str());
        logdbg_ln("Sent 200 response: %s",response);
      }
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
    std::string mac = duckesp::getDuckMacAddress(true);
    request->send(200, "text/html", mac.c_str());
  });

  webServer.on("/wifi", HTTP_GET, [&](AsyncWebServerRequest* request) {
   request->send(200, "text/html", wifi_page);
   
 });

  webServer.on("/changeSSID", HTTP_POST, [&](AsyncWebServerRequest* request) {
    int paramsNumber = request->params();
    std::string val = "";
    std::string ssid = "";
    std::string password = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);

      std::string name = p->name().c_str();
      std::string value = p->value().c_str();

      if (name == "ssid") {
        ssid = p->value().c_str();
      } else if (name == "pass") {
        password = p->value().c_str();
      }
    }

    if (ssid != "" && password != "") {
      setupInternet(ssid, password);
      this->ssid = ssid;
      this->password = password;
      duckutils::saveWifiCredentials(ssid, password);
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

  loginfo_ln("Created Wifi Access Point");
  return DUCK_ERR_NONE;
}

int DuckNet::setupDns() {
  bool success = dnsServer.start(DNS_PORT, "*", apIP);

  if (!success) {
    logerr_ln("ERROR dns server start failed");
    return DUCKDNS_ERR_STARTING;
  }

  success = MDNS.begin(DNS);
  
  if (!success) {
    logerr_ln("ERROR dns server begin failed");
    return DUCKDNS_ERR_STARTING;
  }

  loginfo_ln("Created local DNS");
  MDNS.addService("http", "tcp", CDPCFG_WEB_PORT);

  return DUCK_ERR_NONE;
}

int DuckNet::loadWiFiCredentials(){
  setSsid(duckutils::loadWifiSsid());
  setPassword(duckutils::loadWifiPassword());

  if (ssid.length() == 0 || password.length() == 0){
    loginfo_ln("ERROR setupInternet: Stored SSID and PASSWORD empty");
    return DUCK_ERR_SETUP;
  } else{
    loginfo_ln("Setup Internet with saved credentials");
    setupInternet(ssid, password);
  }
  return DUCK_ERR_NONE;
}


int DuckNet::setupInternet(std::string ssid, std::string password) 
{
  const uint32_t WIFI_CONNECTION_TIMEOUT_MS = 1500;

  int rc = DUCK_ERR_NONE;
  this->ssid = ssid;
  this->password = password;
  // Check if SSID is available
  if (!ssidAvailable(ssid)) {
    logerr_ln("ERROR setupInternet: %s is not available. Please check the provided ssid and/or passwords", ssid.c_str());
    return DUCK_INTERNET_ERR_SSID;
  }

  //  Connect to Access Point
  loginfo_ln("setupInternet: connecting to WiFi access point SSID: %s",ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  // We need to wait here for the connection to estanlish. Otherwise the WiFi.status() may return a false negative
  loginfo_ln("setupInternet: Waiting for connect results for ", ssid.c_str());
  WiFi.waitForConnectResult(WIFI_CONNECTION_TIMEOUT_MS);

  if (WiFi.status() == WL_CONNECTED) {
    loginfo_ln("Duck connected to internet!");
    rc = DUCK_ERR_NONE;
  } else {
    logerr_ln("ERROR setupInternet: failed to connect to %s (status: %d)", ssid.c_str(), WiFi.status());
    rc = DUCK_INTERNET_ERR_CONNECT;
  };

  return rc;

}

bool DuckNet::ssidAvailable(std::string val) {
  int n = WiFi.scanNetworks();
  
  if (n == 0 || ssid == "") {
    logdbg_ln("Networks found: %d", n);
  } else {
    logdbg_ln("Networks found: %d", n);
    if (val == "") {
      val = ssid;
    }
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) == val.c_str()) {
        logdbg_ln("Given ssid is available!");
        return true;
      }
      delay(AP_SCAN_INTERVAL_MS);
    }
  }
  loginfo_ln("No ssid available");

  return false;
}

void DuckNet::saveChannel(int val){

    EEPROM.begin(512);
    EEPROM.write(CDPCFG_EEPROM_CHANNEL_VALUE, val);
    EEPROM.commit();
    loginfo_ln("Wrote channel val to EEPROM %d", val);
    
}

void DuckNet::loadChannel(){
    EEPROM.begin(512);
    int val = EEPROM.read(CDPCFG_EEPROM_CHANNEL_VALUE);
    duck->setChannel(val);
    loginfo_ln("Read channel val to EEPROM, setting channel: %d", val);
}

void DuckNet::setSsid(std::string val) { ssid = val; }

void DuckNet::setPassword(std::string val) { password = val; }

std::string DuckNet::getSsid() { return ssid; }

std::string DuckNet::getPassword() { return password; }

#endif