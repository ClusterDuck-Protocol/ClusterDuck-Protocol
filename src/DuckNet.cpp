#include "include/DuckNet.h"

DuckNet::DuckNet(BloomFilter *filter): bloomFilter(filter) {
}

#ifndef CDPCFG_WIFI_NONE

IPAddress apIP(CDPCFG_AP_IP1, CDPCFG_AP_IP2, CDPCFG_AP_IP3, CDPCFG_AP_IP4);
AsyncWebServer webServer(CDPCFG_WEB_PORT);
AsyncEventSource events("/events");

DNSServer DuckNet::dnsServer;

const char* DuckNet::DNS = "duck";
const byte DuckNet::DNS_PORT = 53;

void DuckNet::setDeviceId(std::array<byte,8> devId) {
    std::copy(devId.begin(), devId.end(), deviceId.begin());
}

int DuckNet::setupWebServer(bool createCaptivePortal, std::string html) {
  loginfo_ln("Setting up Web Server");

  if (txPacket == nullptr) {
    txPacket = new DuckPacket(deviceId);
  }

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
  
  webServer.on("/flipDetector", HTTP_GET, [&](AsyncWebServerRequest* request) {
    //Run flip method
    duckutils::flipDetectState();
    request->send(200, "text/plain", "Success");
  });

  webServer.on("/flipDecrypt", HTTP_GET, [&](AsyncWebServerRequest* request) {
    //Flip Decrypt State
    loginfo_ln("Flipping Decrypt");

    //TODO: Don't use duck for everything
    //duck->setDecrypt(!duck->getDecrypt());
    //loginfo_ln("Decrypt is now: %d", duck->getDecrypt());
    request->send(200, "text/plain", "Success");
  });

  webServer.on("/setChannel", HTTP_POST, [&](AsyncWebServerRequest* request) {
    int paramNum = 0;
    const AsyncWebParameter* p = request->getParam(paramNum);
    logdbg_ln("%s : %d", p->name(), p->value());
    int val = std::atoi(p->value().c_str());
    //TODO: don't use duck for everything
    //duck->setChannel(val);
    saveChannel(val);

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
    std::vector<byte> muid;

    std::vector<byte> data;
    data.insert(data.end(), val.begin(), val.end());
    //TODO: send the correct ducktype. Probably need the ducktype when DuckNet is created or setup
    txPacket->prepareForSending(bloomFilter, ZERO_DUID, DuckType::UNKNOWN, topics::cpm, data );
    err = duckRadio.sendData(txPacket->getBuffer());

    switch (err) {
      case DUCK_ERR_NONE:
      {
        std::string response = "{\"muid\":\"" + duckutils::toString(muid) + "\"}";
        request->send(200, "text/html", response.c_str());
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

void DuckNet::saveChannel(int val){

    EEPROM.begin(512);
    EEPROM.write(CDPCFG_EEPROM_CHANNEL_VALUE, val);
    EEPROM.commit();
    loginfo_ln("Wrote channel val to EEPROM %d", val);
    
}

void DuckNet::loadChannel(){
    EEPROM.begin(512);
    int val = EEPROM.read(CDPCFG_EEPROM_CHANNEL_VALUE);
    //TODO: don't use duck for everything
    //duck->setChannel(val);
    loginfo_ln("Read channel val to EEPROM, setting channel: %d", val);
}

void DuckNet::setSsid(std::string val) { ssid = val; }

void DuckNet::setPassword(std::string val) { password = val; }

std::string DuckNet::getSsid() { return ssid; }

std::string DuckNet::getPassword() { return password; }

#endif