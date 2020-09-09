#include "ClusterDuck.h"
#include "DuckEsp.h"


ClusterDuck::ClusterDuck() {
   duckutils::setDuckInterrupt(true);
  }

void ClusterDuck::begin(int baudRate) {
  Serial.begin(baudRate);
  Serial.print("[CD] Serial start ");
  Serial.println(baudRate, DEC);
}

void ClusterDuck::setDeviceId(String deviceId) {
  _deviceId = deviceId;
  _duckNet->setDeviceId(_deviceId);
}


void ClusterDuck::setupDisplay(String deviceType) {

  _duckDisplay->setupDisplay();

#ifdef CDPCFG_OLED_64x32
  // small display 64x32
  _duckDisplay->setCursor(0, 2);
  _duckDisplay->print("((>.<))");

  _duckDisplay->setCursor(0, 4);
  _duckDisplay->print("DT: " + deviceType);

  _duckDisplay->setCursor(0, 5);
  _duckDisplay->print("ID: " + _deviceId);

#else
  // default display size 128x64
  _duckDisplay->setCursor(0, 1);
  _duckDisplay->print("    ((>.<))    ");

  _duckDisplay->setCursor(0, 2);
  _duckDisplay->print("  Project OWL  ");

  _duckDisplay->setCursor(0, 4);
  _duckDisplay->print("Device: " + deviceType);

  _duckDisplay->setCursor(0, 5);
  _duckDisplay->print("Status: Online");

  _duckDisplay->setCursor(0, 6);
  _duckDisplay->print("ID:     " + _deviceId);

  _duckDisplay->setCursor(0, 7);
  _duckDisplay->print(duckMac(false));
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

  int err = _duckLora->setupLoRa(config, _deviceId);

  if (err == ERR_NONE) {
    Serial.println("[Duck] Listening for quacks");
    return;
  }

  if (err == DUCKLORA_ERR_BEGIN) {
    _duckDisplay->drawString(true, 0, 0, "Starting LoRa failed!");
    Serial.printf("[CD] Starting LoRa Failed. err: %d\n", err);
    duckesp::restartDuck();
  }
  if (err == DUCKLORA_ERR_SETUP) {
    Serial.printf("[CD] Failed to setup Lora. err: %d\n", err);
    while (true)
      ;
  }
  if (err == DUCKLORA_ERR_RECEIVE) {
    Serial.printf("[CD] Failed to start receive. err: %d\n", err);
    duckesp::restartDuck();
  }
}

//============= Used for receiving LoRa packets ==========
volatile bool receivedFlag = false;

void ClusterDuck::setFlag(void) {
  // check if the interrupt is enabled
  if (!duckutils::getDuckInterrupt()) {
    return;
  }
  Serial.println("[CD] setFlag: packet received");

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


void ClusterDuck::setupWebServer(bool createCaptivePortal) {
  _duckNet->setupWebServer(createCaptivePortal);
}

void ClusterDuck::setupWifiAp(const char* AP) {
  _duckNet->setupWifiAp(AP);
}

void ClusterDuck::setupDns() {
  _duckNet->setupDns();
}

void ClusterDuck::setupInternet(String SSID, String PASSWORD) {
  _duckNet->setupInternet(SSID, PASSWORD);
}

bool ClusterDuck::ssidAvailable(String val) {
  bool available = _duckNet->ssidAvailable(val);
  return (available);
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
    duckutils::setDuckInterrupt(false);
    int pSize = handlePacket();
    Serial.print("Duck] runDetect pSize ");
    Serial.println(pSize);
    if (pSize > 0) {
      for (int i = 0; i < pSize; i++) {

        if (_duckLora->getTransmitedByte(i) == iamhere_B) {
          val = _duckLora->getRSSI();
        }
      }
    }
    duckutils::setDuckInterrupt(true);
    Serial.println("Duck] runDetect startReceive");
    startReceive();
  }
  return val;
}

void ClusterDuck::processPortalRequest() { 
  _duckNet->getDnsServer().processNextRequest();  
  }

void ClusterDuck::setupMamaDuck() {
  setupDisplay("Mama");
  setupLoRa();
  setupWifiAp();
  setupDns();
  setupWebServer(true);
  setupOTA();

  Serial.println("MamaDuck Online");

  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);
  duckutils::getTimer().every(CDPCFG_MILLIS_REBOOT, reboot);
}

void ClusterDuck::runMamaDuck() {
  ArduinoOTA.handle();
  if (duckutils::getDuckInterrupt()) {
    duckutils::getTimer().tick();
  }

  // Mama ducks can also receive packets from other ducks
  // Here we check whether a packet needs to be relayed or not
  // For safe processing of the received packet we make sure
  // to disable interrupts, before handling the received packet.
  if (receivedFlag) { 
    receivedFlag = false;
    duckutils::setDuckInterrupt(false);
    int pSize = _duckLora->handlePacket();
    Serial.print("[Duck] runMamaDuck rcv packet: pSize ");
    Serial.println(pSize);
    if (pSize > 0) {
      String msg = _duckLora->getPacketData(pSize);
      _duckLora->resetPacketIndex();
      if (msg != "ping" && !idInPath(_lastPacket.path)) {
        Serial.println("[Duck] runMamaDuck relay packet");
        _duckLora->sendPayloadStandard(_lastPacket.payload, _lastPacket.topic,
                                      _lastPacket.senderId,
                                      _lastPacket.messageId, _lastPacket.path);
        _duckLora->resetTransmissionBuffer();
      }
    } else {
      // discard any unrecognized packets
      _duckLora->resetTransmissionBuffer();
    }
    duckutils::setDuckInterrupt(true);
    Serial.println("runMamaDuck startReceive");
    startReceive();
  }

  processPortalRequest();
}

int ClusterDuck::handlePacket() {
  Serial.println("[ClusterDuck] handling LoRa packet");
  return _duckLora->handlePacket();
}

void ClusterDuck::sendPayloadStandard(String msg, String topic, String senderId,
                                      String messageId, String path) {
  int err = _duckLora->sendPayloadStandard(msg, topic, senderId, messageId, path );
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[ClusterDuck] Oops! Something went wrong, err = ");
    Serial.println(err);
  }
}

void ClusterDuck::couple(byte byteCode, String outgoing) {
  _duckLora->couple(byteCode, outgoing);
}

bool ClusterDuck::idInPath(String path) {
  Serial.print("[ClusterDuck] idInPath '");
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
  return _duckLora->getPacketData(pSize);
}



// Timer reboot
bool ClusterDuck::reboot(void*) {
  String reboot = "REBOOT";
  Serial.println(reboot);
  // this is needed because reboot() is used  by the timer object as a handler
  DuckLora::getInstance()->sendPayloadStandard(reboot, "boot");
  duckesp::restartDuck();

  return true;
}

bool ClusterDuck::imAlive(void*) {
  String alive = "Health Quack";
  Serial.println(alive);
  DuckLora::getInstance()->sendPayloadStandard(alive, "health");
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
  return _duckLora->getLastPacket();
}

volatile bool ClusterDuck::getFlag() { 
  return receivedFlag; 
}

volatile bool ClusterDuck::getInterrupt() {
  return duckutils::getDuckInterrupt();
}

int ClusterDuck::getRSSI() { 
  return _duckLora->getRSSI(); 
}

String ClusterDuck::getSSID() {
  return _duckNet->getSsid();
}

String ClusterDuck::getPassword() {
  return _duckNet->getPassword();
}

void ClusterDuck::setSSID(String val) {
  _duckNet->setSsid(val);
}

void ClusterDuck::setPassword(String val) {
  _duckNet->setPassword(val);
}

void ClusterDuck::flipFlag() {
  if (receivedFlag == true) {
    receivedFlag = false;
  } else {
    receivedFlag = true;
  }
}

void ClusterDuck::flipInterrupt() {
  duckutils::setDuckInterrupt(!duckutils::getDuckInterrupt());
}

void ClusterDuck::startReceive() {
  int err = _duckLora->startReceive();

  if (err != DUCKLORA_ERR_NONE) {
    Serial.println("[ClusterDuck] Restarting Duck...");
    duckesp::restartDuck();
  }
}

void ClusterDuck::startTransmit() {
  int err = _duckLora->startTransmit();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[CD] Oops! Lora transmission failed, err = ");
    Serial.print(err);
  }
}

void ClusterDuck::ping() {
  int err = _duckLora->ping();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[CD] Oops! Lora ping failed, err = ");
    Serial.print(err);
  }
}

// Setup LED
void ClusterDuck::setupLED() { _duckLed->setupLED(); }

void ClusterDuck::setColor(int red, int green, int blue) {
  _duckLed->setColor(red, green, blue);
}

int ClusterDuck::_rssi = 0;
float ClusterDuck::_snr;
long ClusterDuck::_freqErr;
int ClusterDuck::_availableBytes;
int ClusterDuck::_packetSize = 0;

Packet ClusterDuck::_lastPacket;

String ClusterDuck::portal = MAIN_page;
