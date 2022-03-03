#include "include/DuckNet.h"

#include <Update.h>

#include "DuckLogger.h"
#include "include/Duck.h"

DuckNet::DuckNet(Duck* duckIn):
  duck(duckIn)
{}

#ifndef CDPCFG_WIFI_NONE
IPAddress apIP(CDPCFG_AP_IP1, CDPCFG_AP_IP2, CDPCFG_AP_IP3, CDPCFG_AP_IP4);
AsyncWebServer webServer(CDPCFG_WEB_PORT);
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

String DuckNet::getMuidStatusMessage(muidStatus status) {
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
    const char * str = "__FILE__:__LINE__ error: Unrecognized muidStatus";
    logerr(str);
    return str;
  }
}

String DuckNet::getMuidStatusString(muidStatus status) {
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

String DuckNet::createMuidResponseJson(muidStatus status) {
  String statusStr = getMuidStatusString(status);
  String message = getMuidStatusMessage(status);
  return "{\"status\":\"" + statusStr + "\", \"message\":\"" + message + "\"}";
}

int DuckNet::setupWebServer(bool createCaptivePortal, String html) {
  loginfo("Setting up Web Server");

  if (html == "") {
    logdbg("Web Server using main page");
    portal = home_page;
  } else {
    logdbg("Web Server using custom main page");
    portal = html;
  }
  webServer.onNotFound([&](AsyncWebServerRequest* request) {
    logwarn("DuckNet - onNotFound: " + request->url());
    request->send(200, "text/html", portal);
  });

  webServer.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", portal);
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
    loginfo("Flipping Decrypt");

    duck->setDecrypt(!duck->getDecrypt());
    loginfo("Decrypt is now: ");
    loginfo(duck->getDecrypt());
    request->send(200, "text/plain", "Success");
  });

  webServer.on("/setChannel", HTTP_POST, [&](AsyncWebServerRequest* request) {
    AsyncWebParameter* p = request->getParam(0);
    logdbg(p->name() + ": " + p->value());
    int val = std::atoi(p->value().c_str());
    duck->setChannel(val);
    saveChannel(val);

    request->send(200, "text/plain", "Success");
  });

  // webServer.on("/changeControlPassword", HTTP_POST, [&](AsyncWebServerRequest* request) {
  //   int paramsNumber = request->params();
  //   String val = "";
  //   String ssid = "";
  //   String password = "";
  //   String newSsid = "";
  //   String newPassword = "";

  //   for (int i = 0; i < paramsNumber; i++) {
  //     AsyncWebParameter* p = request->getParam(i);

  //     String name = String(p->name());
  //     String value = String(p->value());

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
      duck->updateFirmware(filename, index, data, len, final);
      // TODO: error/exception handling
      // TODO: request->send
    });

  webServer.on("/muidStatus.json", HTTP_GET, [&](AsyncWebServerRequest* request) {
    logdbg(request->url());

    String muid;
    int paramsNumber = request->params();
    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);
      logdbg(p->name() + ": " + p->value());
      if (p->name() == "muid") {
        muid = p->value();
      }
    }

    std::vector<byte> muidVect = {muid[0], muid[1], muid[2], muid[3]};
    muidStatus status = duck->getMuidStatus(muidVect);

    String jsonResponse = createMuidResponseJson(status);
    switch (status) {
    case invalid:
      request->send(400, "text/json", jsonResponse);
      break;
    case unrecognized:
    case not_acked:
    case acked:
      request->send(200, "text/json", jsonResponse);
      break;
    }
  });

  // Captive Portal form submission
  webServer.on("/formSubmit.json", HTTP_POST, [&](AsyncWebServerRequest* request) {
    loginfo("Submitting Form to /formSubmit.json");

    int err = DUCK_ERR_NONE;

    int paramsNumber = request->params();
    String val = "";
    String clientId = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);
      logdbg(p->name() + ": " + p->value());

      if (p->name() == "clientId") {
        clientId = p->value();
      } else {
        val = val + p->value().c_str() + "*";
      }
    }

    clientId.toUpperCase();
    val = "[" + clientId + "]" + val;
    std::vector<byte> muid;
    err = duck->sendData(topics::cpm, val, ZERO_DUID, &muid);

    switch (err) {
      case DUCK_ERR_NONE:
      {
        String response = "{\"muid\":\"" + duckutils::toString(muid) + "\"}";
        request->send(200, "text/html", response);
        logdbg("Sent 200 response: " + response);
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

    Serial.println(ssid);
    Serial.println(password);

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

int DuckNet::loadWiFiCredentials(){
  setSsid(duckutils::loadWifiSsid());
  setPassword(duckutils::loadWifiPassword());

  if (ssid.length() == 0 || password.length() == 0){
    loginfo("ERROR setupInternet: Stored SSID and PASSWORD empty");
    return DUCK_ERR_SETUP;
  } else{
    loginfo("Setup Internet with saved credentials");
    setupInternet(ssid, password);
  }
  return DUCK_ERR_NONE;
}


int DuckNet::setupInternet(String ssid, String password) {
  this->ssid = ssid;
  this->password = password;


  // // Check if SSID is available
  // if (!ssidAvailable(ssid)) {
  //   logerr("ERROR setupInternet: " + ssid + " is not available. Please check the provided ssid and/or passwords");
  //   return DUCK_INTERNET_ERR_SSID;
  // }



  //  Connect to Access Point
  logdbg("setupInternet: connecting to WiFi access point SSID: " + ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  // We need to wait here for the connection to estanlish. Otherwise the WiFi.status() may return a false negative
  // WiFi.waitForConnectResult();
  delay(100);

  //TODO: Handle bad password better
  if(WiFi.status() != WL_CONNECTED) {
    logerr("ERROR setupInternet: failed to connect to " + ssid);
    return DUCK_INTERNET_ERR_CONNECT;
  }

  loginfo("Duck connected to internet!");

  return DUCK_ERR_NONE;

}

bool DuckNet::ssidAvailable(String val) {
  int n = WiFi.scanNetworks();
  
  if (n == 0 || ssid == "") {
    logdbg("Networks found: "+String(n));
  } else {
    logdbg("Networks found: "+String(n));
    if (val == "") {
      val = ssid;
    }
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) == val.c_str()) {
        logdbg("Given ssid is available!");
        return true;
      }
      delay(AP_SCAN_INTERVAL_MS);
    }
  }
  loginfo("No ssid available");

  return false;
}

void DuckNet::saveChannel(int val){

    EEPROM.begin(512);
    EEPROM.write(CDPCFG_EEPROM_CHANNEL_VALUE, val);
    EEPROM.commit();
    loginfo("Wrote channel val to EEPROM");
    loginfo(val);
    
}

void DuckNet::loadChannel(){
    EEPROM.begin(512);
    int val = EEPROM.read(CDPCFG_EEPROM_CHANNEL_VALUE);
    duck->setChannel(val);
    loginfo("Read channel val to EEPROM, setting channel: ");
    loginfo(val);
}

void DuckNet::setSsid(String val) { ssid = val; }

void DuckNet::setPassword(String val) { password = val; }

String DuckNet::getSsid() { return ssid; }

String DuckNet::getPassword() { return password; }

#endif