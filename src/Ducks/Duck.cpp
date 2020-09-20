#include "include/Duck.h"
#include "include/DuckEsp.h"

volatile bool Duck::receivedFlag = false;

Duck::Duck(String id, int baudRate) {
    deviceId = id;
    duckutils::setDuckInterrupt(true);
}

void Duck::setupSerial(int baudRate) {
  while (!Serial && millis() < 10000);
  Serial.begin(baudRate);
  Serial.print("[DuckLink] Serial start ");
  Serial.println(baudRate, DEC);
}

void Duck::onPacketReceived(void) {
  // set the received flag to true if we are not already busy processing
  // a received packet
  if (!duckutils::getDuckInterrupt()) {
    return;
  }
  receivedFlag = true;
}

void Duck::setupRadio(float band, int ss, int rst, int di0, int di1, int txPower) {
  Serial.println("[Duck] Setting up Lora");
  LoraConfigParams config;

  config.band = band;
  config.ss = ss;
  config.rst = rst;
  config.di0 = di0;
  config.di1 = di1;
  config.txPower = txPower;
  config.func = Duck::onPacketReceived;
  
   int err = duckLora->setupLoRa(config, deviceId);

  if (err == ERR_NONE) {
    Serial.println("[Duck] Listening for quacks");
    return;
  }

  if (err == DUCKLORA_ERR_BEGIN) {
    Serial.print("[DuckLink] Starting LoRa Failed. err: ");
    Serial.println(err);
    duckesp::restartDuck();
  }
  if (err == DUCKLORA_ERR_SETUP) {
    Serial.print("[DuckLink] Failed to setup Lora. err: ");
    Serial.println(err);
    while (true)
      ;
  }
  if (err == DUCKLORA_ERR_RECEIVE) {
    Serial.print("[DuckLink] Failed to start receive. err: ");
    Serial.println(err);
    duckesp::restartDuck();
  }
}

void Duck::setupWebServer(bool createCaptivePortal, String html) {
  duckNet->setupWebServer(createCaptivePortal, html);
}

void Duck::setupWifi(const char* ap) {
    duckNet->setupWifiAp(ap);
}

void Duck::setupDns() { 
    duckNet->setupDns(); 
}

void Duck::setupInternet(String ssid, String password) {
  duckNet->setupInternet(ssid, password);
}

#ifdef CDPCFG_WIFI_NONE
void Duck::setupOTA() {}
#else
void Duck::setupOTA() {

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
#endif

#ifdef CDPCFG_WIFI_NONE
void Duck::handleOtaUpdate(){}
#else
void Duck::handleOtaUpdate() { 
    ArduinoOTA.handle();
}
#endif

#ifdef CDPCFG_WIFI_NONE
void Duck::processPortalRequest() {}
#else
void Duck::processPortalRequest() {
  duckNet->dnsServer.processNextRequest();
}
#endif

int Duck::sendPayloadStandard(String msg, String topic, String senderId,
                               String messageId, String path) {
  int err =
      duckLora->sendPayloadStandard(msg, topic, senderId, messageId, path);
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[Duck] Oops! Something went wrong, err = ");
    Serial.println(err);
  }
  return err;
}

bool Duck::reboot(void*) {
  String reboot = "REBOOT";
  Serial.println(reboot);
  DuckLora::getInstance()->sendPayloadStandard(reboot, "boot");
  duckesp::restartDuck();

  return true;
}

bool Duck::imAlive(void*) {
  String alive = "Health Quack";
  Serial.println(alive);
  DuckLora::getInstance()->sendPayloadStandard(alive, "health");
  return true;
}

int Duck::startReceive() {
  int err = duckLora->startReceive();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.println("[Duck] Restarting Duck...");
    duckesp::restartDuck();
  }
  return err;
}

int Duck::startTransmit() {
  int err = duckLora->transmitData();
  if (err != DUCKLORA_ERR_NONE) {
    Serial.print("[Duck] Oops! Lora transmission failed, err = ");
    Serial.print(err);
  }
  return err;
}