#include "ClusterDuck.h"
#include "DuckUtils.h"
#include "DuckEsp.h"

volatile bool enableDuckInterrupt;

volatile bool getDuckInterrupt() { return enableDuckInterrupt; }
void setDuckInterrupt(bool interrupt) { enableDuckInterrupt = interrupt; }

void flipInterrupt() {
  if (enableDuckInterrupt == true) {
    enableDuckInterrupt = false;
  } else {
    enableDuckInterrupt = true;
  }
}

IPAddress apIP(CDPCFG_AP_IP1, CDPCFG_AP_IP2, CDPCFG_AP_IP3, CDPCFG_AP_IP4);
AsyncWebServer webServer(CDPCFG_WEB_PORT);

auto tymer = timer_create_default();

//byte transmission[CDPCFG_CDP_BUFSIZE];
//int packetIndex = 0;

String ssid = "";
String password = "";

String ClusterDuck::_deviceId = "";

bool restartRequired = false;
size_t content_len;

// Username and password for /update
const char* http_username = CDPCFG_UPDATE_USERNAME;
const char* http_password = CDPCFG_UPDATE_PASSWORD;

ClusterDuck::ClusterDuck() { setDuckInterrupt(true); }

void ClusterDuck::setDeviceId(String deviceId) { _deviceId = deviceId; }

void ClusterDuck::begin(int baudRate) {
  Serial.begin(baudRate);
  Serial.print("Serial start ");
  Serial.println(baudRate, DEC);
}

void ClusterDuck::setupDisplay(String deviceType) {

  _duckDisplay.setupDisplay();

#ifdef CDPCFG_OLED_64x32
  // small display 64x32
  _duckDisplay.setCursor(0, 2);
  _duckDisplay.print("((>.<))");

  _duckDisplay.setCursor(0, 4);
  _duckDisplay.print("DT: " + deviceType);

  _duckDisplay.setCursor(0, 5);
  _duckDisplay.print("ID: " + _deviceId);

#else
  // default display size 128x64
  _duckDisplay.setCursor(0, 1);
  _duckDisplay.print("    ((>.<))    ");

  _duckDisplay.setCursor(0, 2);
  _duckDisplay.print("  Project OWL  ");

  _duckDisplay.setCursor(0, 4);
  _duckDisplay.print("Device: " + deviceType);

  _duckDisplay.setCursor(0, 5);
  _duckDisplay.print("Status: Online");

  _duckDisplay.setCursor(0, 6);
  _duckDisplay.print("ID:     " + _deviceId);

  _duckDisplay.setCursor(0, 7);
  _duckDisplay.print(duckMac(false));
#endif
}

// Initial LoRa settings
void ClusterDuck::setupLoRa(long BAND, int SS, int RST, int DI0, int DI1,
                            int TxPower) {

  LoraConfigParams config;

  config.band = BAND;
  config.ss = SS;
  config.rst = RST;
  config.di0 = DI0;
  config.di1 = DI1;
  config.txPower = TxPower;
  config.func = setFlag;

  int err = _duckLora.setupLoRa(config, _deviceId);

  if (err == ERR_NONE) {
    Serial.println("[Duck] Listening for quacks");
    return;
  }

  if (err == DUCKLORA_ERR_BEGIN) {
    _duckDisplay.drawString(true, 0, 0, "Starting LoRa failed!");
    Serial.print("[Duck] Starting LoRa Failed!!!");
    Serial.println(err);
    duckesp::restartDuck();
  }
  if (err == DUCKLORA_ERR_SETUP) {
    Serial.print("[Duck] Failed to setup Lora. err = ");
    Serial.println(err);
    while (true)
      ;
  }
  if (err == DUCKLORA_ERR_RECEIVE) {
    Serial.print("[Duck] Failed to start receive. err = ");
    Serial.println(err);
    duckesp::restartDuck();
  }
}

//============= Used for receiving LoRa packets ==========
volatile bool receivedFlag = false;

void ClusterDuck::setFlag(void) {
  // check if the interrupt is enabled
  if (!getDuckInterrupt()) {
    return;
  }
  // we got a packet, set the flag
  receivedFlag = true;
}
// =====================================

// handle the upload of the firmware
void handleFirmwareUpload(AsyncWebServerRequest* request, String filename,
                          size_t index, uint8_t* data, size_t lenght,
                          bool final) {
  // handle upload and update
  if (!index) {
    Serial.printf("Update: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
      Update.printError(Serial);
    }
  }
  // flashing firmware to ESP
  if (lenght) {
    Update.write(data, lenght);
  }
  if (final) {
    if (Update.end(true)) { // true to set the size to the current progress
      Serial.printf("Update Success: %ub written\nRebooting...\n",
                    index + lenght);
    } else {
      Update.printError(Serial);
    }
  }
}

// Setup WebServer
void ClusterDuck::setupWebServer(bool createCaptivePortal) {
  Serial.println("Setting up Web Server");

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
      [](AsyncWebServerRequest* request, String filename, size_t index,
         uint8_t* data, size_t len, bool final) {
        if (!index) {

          _duckLora.standBy();
          Serial.println("Pause Lora");
          Serial.println("startint OTA update");

          content_len = request->contentLength();

          int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {

            Update.printError(Serial);
          }
        }

        if (Update.write(data, len) != len) {
          Update.printError(Serial);
          _duckLora.startReceive();
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
    Serial.println("Submitting Form");

    int paramsNumber = request->params();
    String val = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);
      Serial.printf("%s: %s", p->name().c_str(), p->value().c_str());
      Serial.println();

      val = val + p->value().c_str() + "*";
    }

    _duckLora.sendPayloadStandard(val, "status");

    request->send(200, "text/html", portal);
  });

  webServer.on("/id", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/html", _deviceId);
  });

  webServer.on("/restart", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Restarting...");
    delay(1000);
    duckesp::restartDuck();
  });

  webServer.on("/mac", HTTP_GET, [&](AsyncWebServerRequest* request) {
    String mac = duckMac(true);
    request->send(200, "text/html", mac);
  });

  webServer.on("/wifi", HTTP_GET, [&](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Update Wifi "
                    "Credentials</title></head><body>");
    response->print("<p>Use this page to update your Wifi credentials</p>");

    response->print("<form action='/changeSSID' method='post'>");

    response->print("<label for='ssid'>SSID:</label><br>");
    response->print(
        "<input name='ssid' type='text' placeholder='SSID' /><br><br>");

    response->print("<label for='pass'>Password:</label><br>");
    response->print(
        "<input name='pass' type='text' placeholder='Password' /><br><br>");

    response->print("<input type='submit' value='Submit' />");

    response->print("</form>");

    response->print("</body></html>");
    request->send(response);
  });

  webServer.on("/changeSSID", HTTP_POST, [&](AsyncWebServerRequest* request) {
    int paramsNumber = request->params();
    String val = "";
    String SSID = "";
    String PASSWORD = "";

    for (int i = 0; i < paramsNumber; i++) {
      AsyncWebParameter* p = request->getParam(i);

      String name = String(p->name());
      String value = String(p->value());

      if (name == "ssid") {
        SSID = String(p->value());
      } else if (name == "pass") {
        PASSWORD = String(p->value());
      }
    }

    if (SSID != "" && PASSWORD != "") {
      setupInternet(SSID, PASSWORD);
      request->send(200, "text/plain", "Success");
    } else {
      request->send(500, "text/plain", "There was an error");
    }
  });

  webServer.begin();
}

void ClusterDuck::setupWifiAp(const char* AP) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP);
  delay(200); // wait for 200ms for the access point to start before configuring

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  Serial.println("Created Wifi Access Point");
}

void ClusterDuck::setupDns() {
  dnsServer.start(DNS_PORT, "*", apIP);

  if (!MDNS.begin(DNS)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("Created local DNS");
    MDNS.addService("http", "tcp", CDPCFG_WEB_PORT);
  }
}

void ClusterDuck::setupInternet(String SSID, String PASSWORD) {
  ssid = SSID;
  password = PASSWORD;
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(SSID);

  if (SSID != "" && PASSWORD != "" && ssidAvailable(ssid)) {
    // Connect to Access Point
    WiFi.begin(SSID.c_str(), PASSWORD.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      tymer.tick(); // Advance timer to reboot after awhile
      // TODO: Change this to make sure it is non-blocking for all other
      // processes
    }

    // Connected to Access Point
    Serial.println("");
    Serial.println("DUCK CONNECTED TO INTERNET");
  }
}

bool ClusterDuck::ssidAvailable(
    String val) { // TODO: needs to be cleaned up for null case
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0 || ssid == "") {
    Serial.print(n);
    Serial.println(" networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    if (val == "") {
      val = ssid;
    }
    for (int i = 0; i < n; ++i) {
      Serial.print(WiFi.SSID(i));
      if (WiFi.SSID(i) == val) {
        return true;
      }
      delay(10);
    }
  }
  return false;
}

// Setup OTA
void ClusterDuck::setupOTA() {

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using
    // SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}

// Setup premade DuckLink with default settings
void ClusterDuck::setupDuckLink() {
  setupDisplay("Duck");
  setupLoRa();
  setupWifiAp();
  setupDns();
  setupWebServer(true);
  setupOTA();

  Serial.println("Duck Online");
}

void ClusterDuck::runDuckLink() {
  ArduinoOTA.handle();
  processPortalRequest();
}

void ClusterDuck::setupDetect() {
  setupDisplay("Detector");
  setupLoRa();
  // setupWifiAp(false);
  // setupDns();
  // setupWebServer(false);
  // setupOTA();

  Serial.println("Detector Online");
}

int ClusterDuck::runDetect() {
  ArduinoOTA.handle();
  int val = 0;
  if (receivedFlag) { // If LoRa packet received
    receivedFlag = false;
    setDuckInterrupt(false);
    int pSize = handlePacket();
    Serial.print("runDetect pSize ");
    Serial.println(pSize);
    if (pSize > 0) {
      for (int i = 0; i < pSize; i++) {

        if (_duckLora.getTransmitedByte(i) == iamhere_B) {
          val = _duckLora.getRSSI();
        }
      }
    }
    setDuckInterrupt(true);
    Serial.println("runDetect startReceive");
    startReceive();
  }
  return val;
}

void ClusterDuck::processPortalRequest() { dnsServer.processNextRequest(); }

void ClusterDuck::setupMamaDuck() {
  setupDisplay("Mama");
  setupLoRa();
  setupWifiAp();
  setupDns();
  setupWebServer(true);
  setupOTA();

  Serial.println("MamaDuck Online");

  tymer.every(CDPCFG_MILLIS_ALIVE, imAlive);
  tymer.every(CDPCFG_MILLIS_REBOOT, reboot);
}

void ClusterDuck::runMamaDuck() {
  ArduinoOTA.handle();
  if (getDuckInterrupt()) {
    tymer.tick();
  }

  if (receivedFlag) { // If LoRa packet received
    receivedFlag = false;
    setDuckInterrupt(false);
    int pSize = _duckLora.handlePacket();
    Serial.print("[Duck] runMamaDuck rcv packet: pSize ");
    Serial.println(pSize);
    if (pSize > 0) {
      String msg = _duckLora.getPacketData(pSize);
      _duckLora.resetPacketIndex();
      if (msg != "ping" && !idInPath(_lastPacket.path)) {
        Serial.println("[Duck] runMamaDuck relay packet");
        _duckLora.sendPayloadStandard(_lastPacket.payload, _lastPacket.topic,
                                      _lastPacket.senderId,
                                      _lastPacket.messageId, _lastPacket.path);
        _duckLora.resetTransmissionBuffer();
      }
    } else {
      // Serial.println("Byte code not recognized!");
      _duckLora.resetTransmissionBuffer();
    }
    setDuckInterrupt(true);
    Serial.println("runMamaDuck startReceive");
    startReceive();
  }

  processPortalRequest();
}

int ClusterDuck::handlePacket() {
 return _duckLora.handlePacket();
}

void ClusterDuck::sendPayloadStandard(String msg, String topic, String senderId,
                                      String messageId, String path) {
  int err = _duckLora.sendPayloadStandard(msg, topic, senderId, messageId, path );
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[ClusterDuck] Oops! Something went wrong, err = ");
    Serial.println(err);
  }
}

void ClusterDuck::couple(byte byteCode, String outgoing) {
  _duckLora.couple(byteCode, outgoing);
}

bool ClusterDuck::idInPath(String path) {
  Serial.print("idInPath '");
  Serial.print(path);
  Serial.print("' ");
  String temp = "";
  int len = path.length() + 1;
  char arr[len];
  path.toCharArray(arr, len);

  for (int i = 0; i < len; i++) {
    if (arr[i] == ',' || i == len - 1) {
      if (temp == _deviceId) {
        Serial.println("true");
        return true;
      }
      temp = "";
    } else {
      temp += arr[i];
    }
  }
  Serial.println("false");
  return false;
}

String ClusterDuck::getPacketData(int pSize) {
  return _duckLora.getPacketData(pSize);
}



// Timer reboot
bool ClusterDuck::reboot(void*) {
  String reboot = "REBOOT";
  Serial.println(reboot);
  _duckLora.sendPayloadStandard(reboot, "boot");
  duckesp::restartDuck();

  return true;
}

bool ClusterDuck::imAlive(void*) {
  String alive = "Health Quack";
  Serial.print("imAlive sending");
  _duckLora.sendPayloadStandard(alive, "health");

  return true;
}

// Get Duck MAC address
String ClusterDuck::duckMac(boolean format) {
  return duckesp::getDuckMacAddress(format);
}

// Create a uuid
String ClusterDuck::uuidCreator() {
  return duckutils::createUuid();
}

// Getters
String ClusterDuck::getDeviceId() { return _deviceId; }

Packet ClusterDuck::getLastPacket() {
  return _duckLora.getLastPacket();
}

volatile bool ClusterDuck::getFlag() { 
  return receivedFlag; 
}

volatile bool ClusterDuck::getInterrupt() {
  return getDuckInterrupt();
}

int ClusterDuck::getRSSI() { return _duckLora.getRSSI(); }

String ClusterDuck::getSSID() { return ssid; }

String ClusterDuck::getPassword() { return password; }

// Setter
void ClusterDuck::setSSID(String val) { ssid = val; }

void ClusterDuck::setPassword(String val) { password = val; }

void ClusterDuck::flipFlag() {
  if (receivedFlag == true) {
    receivedFlag = false;
  } else {
    receivedFlag = true;
  }
}

void ClusterDuck::flipInterrupt() { 
  flipInterrupt();
}

void ClusterDuck::startReceive() {
  int err = _duckLora.startReceive();

  if (err != DUCKLORA_ERR_NONE) {
    Serial.println("[ClusterDuck] Restarting Duck...");
    duckesp::restartDuck();
  }
}

void ClusterDuck::startTransmit() {
  int err = _duckLora.startTransmit();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[ClusterDuck] Oops! Lora transmission failed, err = ");
    Serial.print(err);
  }
}

void ClusterDuck::ping() {
  int err = _duckLora.ping();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[ClusterDuck] Oops! Lora ping failed, err = ");
    Serial.print(err);
  }
}

// Setup LED
void ClusterDuck::setupLED() { _duckLed.setupLED(); }

void ClusterDuck::setColor(int red, int green, int blue) {
  _duckLed.setColor(red, green, blue);
}

DNSServer ClusterDuck::dnsServer;
const char* ClusterDuck::DNS = "duck";
const byte ClusterDuck::DNS_PORT = 53;

int ClusterDuck::_rssi = 0;
float ClusterDuck::_snr;
long ClusterDuck::_freqErr;
int ClusterDuck::_availableBytes;
int ClusterDuck::_packetSize = 0;

Packet ClusterDuck::_lastPacket;

String ClusterDuck::portal = MAIN_page;
DuckDisplay ClusterDuck::_duckDisplay;
DuckLed ClusterDuck::_duckLed;
DuckLora ClusterDuck::_duckLora;
