#include "include/Duck.h"
#include "include/DuckEsp.h"

volatile bool Duck::receivedFlag = false;

Duck::Duck() {
  duckutils::setDuckInterrupt(true);
}

Duck::Duck(std::vector<byte> id) {
  duid.insert(duid.end(), id.begin(), id.end());
  duckutils::setDuckInterrupt(true);
}

int Duck::setDeviceId(std::vector<byte> id) {
  if (id.size() > DUID_LENGTH) {
    logerr("ERROR  device id too long rc = " + String(DUCK_ERR_NONE));
    return DUCK_ERR_ID_TOO_LONG;
  }
  if (duid.size() > 0) {
    duid.clear();
  }
  duid.insert(duid.end(), id.begin(), id.end());
  loginfo("setupDeviceId rc = " + String(DUCK_ERR_NONE));
  return DUCK_ERR_NONE;
}

int Duck::setDeviceId(byte* id) {
  if (id == NULL) {
    return DUCK_ERR_SETUP;
  }
  int len = *(&id + 1) - id;
  if (len > DUID_LENGTH) {
    return DUCK_ERR_ID_TOO_LONG;
  }
  duid.assign(id, id + len);
  loginfo("setupDeviceId rc = " + String(DUCK_ERR_NONE));

  return DUCK_ERR_NONE;
}

int Duck::setupSerial(int baudRate) {
  while (!Serial && millis() < 10000);

  Serial.begin(baudRate);
  loginfo("setupSerial rc = " + String(DUCK_ERR_NONE));
  return DUCK_ERR_NONE;
}

void Duck::onPacketReceived(void) {
  // set the received flag to true if we are not already busy processing
  // a received packet
  if (!duckutils::getDuckInterrupt()) {
    return;
  }
  setReceiveFlag(true);
}

int Duck::setupRadio(float band, int ss, int rst, int di0, int di1, int txPower) {
  LoraConfigParams config;

  config.band = band;
  config.ss = ss;
  config.rst = rst;
  config.di0 = di0;
  config.di1 = di1;
  config.txPower = txPower;
  config.func = Duck::onPacketReceived;
  
  int err = duckRadio->setupRadio(config);

  if (err == DUCKLORA_ERR_BEGIN) {
    logerr("ERROR setupRadio. Starting LoRa Failed. rc = " + String(err));
    return err;
  }
  if (err == DUCKLORA_ERR_SETUP) {
    logerr("ERROR setupRadio. Failed to setup Lora. rc = " + String(err));
    return err;
  }
  if (err == DUCKLORA_ERR_RECEIVE) {
    logerr("ERROR setupRadio. Failed to start receive. rc = "+String(err));
    return err;
  }

  txPacket = new DuckPacket(duid);
  rxPacket = new DuckPacket();
  loginfo("setupRadio rc = " + String(DUCK_ERR_NONE));

  return DUCK_ERR_NONE;
}

int Duck::setupWebServer(bool createCaptivePortal, String html) {
  int err = DUCK_ERR_NONE;
  duckNet->setDeviceId(duid);
  err = duckNet->setupWebServer(createCaptivePortal, html);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWebServer rc = " + String(err));
  } else {
    loginfo("setupWebServer OK");
  }
  return err;
}

int Duck::setupWifi(const char* ap) {
  int err = duckNet->setupWifiAp(ap);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWifi rc = " + String(err));
  } else {
    loginfo("setupWifi OK");
  }
  return err;
}

int Duck::setupDns() {
  int err = duckNet->setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupDns rc = " + String(err));
  } else {
    loginfo("setupDns OK");
  }
  return err;
}

int Duck::setupInternet(String ssid, String password) {
  int err = duckNet->setupInternet(ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupInternet rc = " + String(err));
  } else {
    loginfo("setupInternet OK");
  }
  return err;
}

#ifdef CDPCFG_WIFI_NONE
int Duck::setupOTA() {return DUCK_ERR_NONE;}
#else
int Duck::setupOTA() {

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using
    // SPIFFS.end()
    loginfo("setupOTA. Start updating " + type);
  });
  ArduinoOTA.onEnd([]() { loginfo("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    loginfo_f("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    logerr_f("Error[%u]: ", error);
    switch(error) {
      case OTA_AUTH_ERROR:
        logerr("ERROR setupOTA. Auth Failed");
        break;
      case OTA_BEGIN_ERROR:
        logerr("ERROR setupOTA. Begin Failed");
        break;
      case OTA_CONNECT_ERROR:
        logerr("ERROR setupOTA. Connect Failed");
        break;
      case OTA_RECEIVE_ERROR:
        logerr("ERROR setupOTA. Receive Failed");
        break;
      case OTA_END_ERROR:
        logerr("ERROR setupOTA. End Failed");
        break;
      default:
        logerr("ERROR setupOTA. Unknown Error");
    }
  });

  ArduinoOTA.begin();
  loginfo("setupOTA rc = " + String(DUCK_ERR_NONE));
  return DUCK_ERR_NONE;
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

int Duck::sendData(byte topic, const String data) {

  const byte* buffer = (byte*)data.c_str();
  int err = sendData(topic, buffer, data.length());
  return err;
}

int Duck::sendData(byte topic, const std::string data) {
  std::vector<byte> app_data;
  app_data.insert(app_data.end(), data.begin(), data.end());
  int err = sendData(topic, app_data);
  return err;
}

int Duck::sendData(byte topic, const byte* data, int length) {
  std::vector<byte> app_data;
  app_data.insert(app_data.end(), &data[0], &data[length]);
  int err = sendData(topic, app_data);
  return err;
}

int Duck::sendData(byte topic, std::vector<byte> data) {
  String str;
  str.c_str();
  if (topic < reservedTopic::max_reserved) {
    logerr("ERROR send data failed, topic is reserved.");
    return DUCKPACKET_ERR_TOPIC_INVALID;
  }
  int err = txPacket->buildPacketBuffer(topic, data);
  if ( err != DUCK_ERR_NONE) {
    return err;
  }

  int length = txPacket->getBufferLength();
  err = duckRadio->sendData(txPacket->getDataByteBuffer(), length);
  txPacket->reset();
  return err;
}

// TODO: implement this using new packet format
bool Duck::reboot(void*) {
  /*
    String reboot = "REBOOT";
    loginfo(reboot);
    DuckRadio::getInstance()->sendPayloadStandard(reboot, "boot");
    duckesp::restartDuck();
  */
  return true;
}

// TODO: implement this using new packet format
bool Duck::imAlive(void*) {
  /*
  String alive = "Health Quack";
  loginfo(alive);
  DuckRadio::getInstance()->sendPayloadStandard(alive, "health");
  */
  return true;
}

int Duck::startReceive() {
  int err = duckRadio->startReceive();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Restarting Duck...");
    duckesp::restartDuck();
  }
  return err;
}

int Duck::startTransmit() {
  int err = duckRadio->startTransmitData();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Oops! Lora transmission failed, err = " + String(err));
  }
  return err;
}

int Duck::sendPong() {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  txPacket->reset();
  err = txPacket->buildPacketBuffer(reservedTopic::pong, data);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Oops! failed to build pong packet, err = " + err);
    return err;
  }
  err = duckRadio->sendData(txPacket->getDataByteBuffer(), txPacket->getBufferLength());
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Oops! Lora sendData failed, err = " + err);
    return err;
  }
  return err;
}

int Duck::sendPing() {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  txPacket->reset();
  err = txPacket->buildPacketBuffer(reservedTopic::ping, data);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Failed to build ping packet, err = " + err);
    return err;
  }
  err = duckRadio->sendData(txPacket->getDataByteBuffer(),
                            txPacket->getBufferLength());
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR Lora sendData failed, err = " + err);
    return err;
  }
  return err;
}