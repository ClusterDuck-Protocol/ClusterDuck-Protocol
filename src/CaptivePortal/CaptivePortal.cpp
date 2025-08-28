#include "../include/CaptivePortal.h"

#ifndef CDPCFG_WIFI_NONE
int CaptivePortal::setupAccessPoint(const char* ap) {
  bool success;

  success = WiFi.mode(WIFI_AP);
  if (!success) {
    return DUCKWIFI_ERR_AP_CONFIG;
  }

  success = WiFi.softAP(ap);
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

int CaptivePortal::setupDns() {
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
  MDNS.addService("http", "tcp", port);

  return DUCK_ERR_NONE;
}

int CaptivePortal::setupWebServer() {
  loginfo_ln("Setting up Web Server");

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  //HTTP Basic authentication
  webServer.addHandler(&events);
  portal = home_page;

  webServer.onNotFound([&](AsyncWebServerRequest* request) {
    logwarn_ln("CaptivePortal - onNotFound: %s", request->url().c_str());
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

  webServer.on("/success.txt", HTTP_GET, [&](AsyncWebServerRequest* request) {
    loginfo_ln("client connected to CaptivePortal");
    request->send(200, "text/plain", "Success");
  });

  // Captive Portal form submission
  webServer.on("/formSubmit.json", HTTP_POST, [&](AsyncWebServerRequest* request) {
    loginfo_ln("Submitting Form to /formSubmit.json");

    int err = DUCK_ERR_NONE;

    int paramsNumber = request->params();
    std::string val = "";
    std::string clientId = "";

    for (int i = 0; i < paramsNumber; i++) {
      const AsyncWebParameter* p = request->getParam(i);
      logdbg_ln("%s : %s", p->name().c_str(), p->value().c_str());

      if (p->name() == "clientId") {
        clientId = p->value().c_str();
      } else {
        val = val + p->value().c_str() + "*";
      }
    }
    clientId = duckutils::toUpperCase(clientId); 
    val = "[" + clientId + "]" + val;

    std::vector<uint8_t> data;
    data.insert(data.end(), val.begin(), val.end());
    
     err = duck.sendData(topics::cpm, val);
 

    switch (err) {
      case DUCK_ERR_NONE:
      {
        std::string response = "Ok";
        request->send(200, "application/json", response.c_str());
        logdbg_ln("Sent 200 response: %s",response.c_str());
      }
      break;
      case DUCKLORA_ERR_MSG_TOO_LARGE:
      request->send(413, "text/html", "Message payload too big!");
      break;
      case DUCKLORA_ERR_HANDLE_PACKET:
      request->send(400, "text/html", "BadRequest");
      break;
      default:
      request->send(500, "text/html", "Unknown error.");
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

 webServer.on("/controlpanel", HTTP_GET, [&](AsyncWebServerRequest* request) {
   request->send(200, "text/html", controlPanel);
   
 });

  webServer.on("/changeSSID", HTTP_POST, [&](AsyncWebServerRequest* request) {
    int paramsNumber = request->params();
    std::string val = "";
    std::string ssid = "";
    std::string password = "";

    for (int i = 0; i < paramsNumber; i++) {
      const AsyncWebParameter* p = request->getParam(i);

      std::string name = p->name().c_str();
      std::string value = p->value().c_str();

      if (name == "ssid") {
        ssid = p->value().c_str();
      } else if (name == "pass") {
        password = p->value().c_str();
      }
    }

    if (ssid != "" && password != "") {
      // duck.joinNetwork(ssid, password);
      // duck.saveWifiCredentials(ssid, password);
      request->send(200, "text/plain", "Success");
    } else {
      request->send(500, "text/plain", "There was an error");
    }
  });

  webServer.begin();

  return DUCK_ERR_NONE;
}

#endif