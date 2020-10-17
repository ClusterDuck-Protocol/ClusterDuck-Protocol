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
    logerr_ln("ERROR device id too long rc = " + String(DUCK_ERR_NONE));
    return DUCK_ERR_ID_TOO_LONG;
  }
  if (duid.size() > 0) {
    duid.clear();
  }
  duid.insert(duid.end(), id.begin(), id.end());
  loginfo_ln("setupDeviceId rc = " + String(DUCK_ERR_NONE));
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
  loginfo_ln("setupDeviceId rc = " + String(DUCK_ERR_NONE));

  return DUCK_ERR_NONE;
}

int Duck::setupSerial(int baudRate) {
  while (!Serial && millis() < 10000);

  Serial.begin(baudRate);
  loginfo_ln("setupSerial rc = " + String(DUCK_ERR_NONE));
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
    logerr_ln("setupRadio. Starting LoRa Failed. rc = " + String(err));
    return err;
  }
  if (err == DUCKLORA_ERR_SETUP) {
    logerr_ln("setupRadio. Failed to setup Lora. rc = " + String(err));
    return err;
  }
  if (err == DUCKLORA_ERR_RECEIVE) {
    logerr_ln("setupRadio. Failed to start receive. rc = "+String(err));
    return err;
  }

  txPacket = new DuckPacket(duid);
  rxPacket = new DuckPacket();
  loginfo_ln("setupRadio rc = " + String(DUCK_ERR_NONE));

  return DUCK_ERR_NONE;
}

int Duck::setupWebServer(bool createCaptivePortal, String html) {
  int err = DUCK_ERR_NONE;
  duckNet->setDeviceId(duid);
  err = duckNet->setupWebServer(createCaptivePortal, html);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupWebServer rc = " + String(err));
  } else {
    loginfo_ln("setupWebServer OK");
  }
  return err;
}

int Duck::setupWifi(const char* ap) {
  int err = duckNet->setupWifiAp(ap);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupWifi rc = " + String(err));
  } else {
    loginfo_ln("setupWifi OK");
  }
  return err;
}

int Duck::setupDns() {
  int err = duckNet->setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupDns rc = " + String(err));
  } else {
    loginfo_ln("setupDns OK");
  }
  return err;
}

int Duck::setupInternet(String ssid, String password) {
  int err = duckNet->setupInternet(ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("setupInternet rc = " + String(err));
  } else {
    loginfo_ln("setupInternet OK");
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
    loginfo_ln("setupOTA. Start updating " + type);
  });
  ArduinoOTA.onEnd([]() { loginfo_ln("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    loginfo_f("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    logerr_f("Error[%u]: ", error);
    switch(error) {
      case OTA_AUTH_ERROR:
        logerr_ln("setupOTA. Auth Failed");
        break;
      case OTA_BEGIN_ERROR:
        logerr_ln("setupOTA. Begin Failed");
        break;
      case OTA_CONNECT_ERROR:
        logerr_ln("setupOTA. Connect Failed");
        break;
      case OTA_RECEIVE_ERROR:
        logerr_ln("setupOTA. Receive Failed");
        break;
      case OTA_END_ERROR:
        logerr_ln("setupOTA. End Failed");
        break;
      default:
        logerr_ln("setupOTA. Unknown Error");
    }
  });

  ArduinoOTA.begin();
  loginfo_ln("setupOTA rc = " + String(DUCK_ERR_NONE));
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

int Duck::sendData(byte topic, std::vector<byte> data) {
  if (topic < reservedTopic::max_reserved) {
    logerr_ln("send data failed, topic is reserved.");
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
    loginfo_ln(reboot);
    DuckRadio::getInstance()->sendPayloadStandard(reboot, "boot");
    duckesp::restartDuck();
  */
  return true;
}

// TODO: implement this using new packet format
bool Duck::imAlive(void*) {
  /*
  String alive = "Health Quack";
  loginfo_ln(alive);
  DuckRadio::getInstance()->sendPayloadStandard(alive, "health");
  */
  return true;
}

int Duck::startReceive() {
  int err = duckRadio->startReceive();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("Restarting Duck...");
    duckesp::restartDuck();
  }
  return err;
}

int Duck::startTransmit() {
  int err = duckRadio->startTransmitData();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("Oops! Lora transmission failed, err = " + String(err));
  }
  return err;
}

int Duck::sendPong() {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  txPacket->reset();
  err = txPacket->buildPacketBuffer(reservedTopic::pong, data);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("Oops! failed to build pong packet, err = " + err);
    return err;
  }
  err = duckRadio->sendData(txPacket->getDataByteBuffer(), txPacket->getBufferLength());
  if (err != DUCK_ERR_NONE) {
    logerr_ln("Oops! Lora sendData failed, err = " + err);
    return err;
  }
  return err;
}