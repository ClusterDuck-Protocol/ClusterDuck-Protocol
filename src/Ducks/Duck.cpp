#include "include/Duck.h"

#include <cassert>
#include "../CdpPacket.h"
#include "include/bloomfilter.h"
#include "include/DuckCrypto.h"
#include "include/DuckEsp.h"
#include "include/DuckNet.h"

const int MEMORY_LOW_THRESHOLD = PACKET_LENGTH + sizeof(CdpPacket);

Duck::Duck(std::string name):
  
  filter(), // initialized filter with default values
  duckNet(new DuckNet(&filter))
{
  duckName = name;
}

Duck::~Duck() {
  if (txPacket != NULL) {
    delete txPacket;
  }
  if (rxPacket != NULL) {
    delete rxPacket;
  }

  delete duckNet;
}

void Duck::setEncrypt(bool state) {
  duckcrypto::setEncrypt(state);
}

bool Duck::getEncrypt() {
  return duckcrypto::getState();
}

bool Duck::getDecrypt() {
  return duckcrypto::getDecrypt();
}

void Duck::setDecrypt(bool state) {
  duckcrypto::setDecrypt(state);
}

void Duck::setAESKey(uint8_t newKEY[32]) {
  duckcrypto::setAESKey(newKEY);
}

void Duck::setAESIv(uint8_t newIV[16]) {
  duckcrypto::setAESIV(newIV);
}

void Duck::encrypt(uint8_t* text, uint8_t* encryptedData, size_t inc) {
  duckcrypto::encryptData(text, encryptedData, inc);
}

void Duck::decrypt(uint8_t* encryptedData, uint8_t* text, size_t inc) {
  duckcrypto::decryptData(encryptedData, text, inc);
}

void Duck::logIfLowMemory() {
  if (duckesp::getMinFreeHeap() < MEMORY_LOW_THRESHOLD
    || duckesp::getMaxAllocHeap() < MEMORY_LOW_THRESHOLD
  ) {
    //logwarn_ln("WARNING heap memory is low");
  }
}

int Duck::setDeviceId(std::array<byte,8>& id) {
    std::copy(id.begin(), id.end(),duid.begin());
  loginfo_ln("setupDeviceId rc = %d",DUCK_ERR_NONE);
  return DUCK_ERR_NONE;
}


int Duck::setDeviceId(byte* id) {
    if (id == nullptr) {
        logerr_ln("ERROR  device id empty or address invalid = %d", DUCK_ERR_SETUP);
        return DUCK_ERR_SETUP;
    }
    int len = *(&id + 1) - id;
    if (len > DUID_LENGTH) {
        logerr_ln("ERROR  device id too long rc = %d", DUCK_ERR_ID_TOO_LONG);
        return DUCK_ERR_ID_TOO_LONG;
    }
    std::copy(id, id + len, duid.begin());
    loginfo_ln("setupDeviceId rc = %d",DUCK_ERR_NONE);
    return DUCK_ERR_NONE;
}

int Duck::setDeviceId(std::string& id) {
    if (id.size() != DUID_LENGTH) {
        logerr_ln("ERROR  device id too long rc = %d", DUCK_ERR_ID_TOO_LONG);
        return DUCK_ERR_ID_TOO_LONG;
    }
    std::copy(id.begin(), id.end(),duid.begin());
    loginfo_ln("setupDeviceId rc = %d", DUCK_ERR_NONE);
    return DUCK_ERR_NONE;
}

int Duck::setupSerial(int baudRate) {
  // This gives us 10 seconds to do a hard reset if the board is in a bad state after power cycle
  while (!Serial && millis() < 10000);

  Serial.begin(baudRate);
  loginfo_ln("setupSerial rc = %d",DUCK_ERR_NONE);
  loginfo_ln("Running CDP Version: %s",getCDPVersion().c_str());
  return DUCK_ERR_NONE;
}

// TODO: use LoraConfigParams directly as argument to setupRadio
int Duck::setupRadio(float band, int ss, int rst, int di0, int di1,
                     int txPower, float bw, uint8_t sf, uint8_t gain) {
  LoraConfigParams config;

  config.band = band;
  config.ss = ss;
  config.rst = rst;
  config.di0 = di0;
  config.di1 = di1;
  config.txPower = txPower;
  config.bw = bw;
  config.sf = sf;
  config.gain = gain;
  config.func = DuckRadio::onInterrupt;

  int err = duckRadio.setupRadio(config);

  if (err == DUCKLORA_ERR_BEGIN) {
    logerr_ln("ERROR setupRadio. Starting LoRa Failed. rc = %d",err);
    return err;
  }
  if (err == DUCKLORA_ERR_SETUP) {
    logerr_ln("ERROR setupRadio. Setup LoRa Failed. rc = %d",err);
    return err;
  }
  if (err == DUCKLORA_ERR_RECEIVE) {
    logerr_ln("ERROR setupRadio. Receive LoRa Failed. rc = %d",err);
    return err;
  }

  txPacket = new DuckPacket(duid);
  rxPacket = new DuckPacket();
  loginfo_ln("setupRadio rc = %d",DUCK_ERR_NONE);

  return DUCK_ERR_NONE;
}

void Duck::setSyncWord(byte syncWord) {
  duckRadio.setSyncWord(syncWord);
}

void Duck::setChannel(int channelNum) {
  duckRadio.setChannel(channelNum);
}

int Duck::setupWebServer(bool createCaptivePortal, std::string html) {
  int err = DUCK_ERR_NONE;
  duckNet->setDeviceId(duid);
  err = duckNet->setupWebServer(createCaptivePortal, html);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWebServer %d",err);
  } else {
    loginfo_ln("setupWebServer OK");
  }
  return err;
}

int Duck::setupWifi(const char* ap) {
  int err = duckNet->setupWifiAp(ap);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupWifi rc = %d",DUCK_ERR_NONE);
  } else {
    loginfo_ln("setupWifi OK");
  }
  return err;
}

int Duck::setupDns() {
  int err = duckNet->setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupDns rc = %d",DUCK_ERR_NONE);
  } else {
    loginfo_ln("setupDns OK");
  }
  return err;
}

int Duck::setupInternet(std::string ssid, std::string password) {
  int err = duckNet->setupInternet(ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR setupInternet rc = %d",DUCK_ERR_NONE);
  } else {
    loginfo_ln("setupInternet OK");
  }
  return err;
}

#ifdef CDPCFG_WIFI_NONE
void Duck::processPortalRequest() {}
#else
void Duck::processPortalRequest() { duckNet->dnsServer.processNextRequest(); }
#endif

int Duck::sendData(byte topic, const std::string data,
  const std::array<byte,8> targetDevice, std::array<byte,8> * outgoingMuid)
{
  std::vector<byte> app_data;
  app_data.insert(app_data.end(), data.begin(), data.end());
  int err = sendData(topic, app_data, targetDevice, outgoingMuid);
  return err;
}

int Duck::sendData(byte topic, const byte* data, int length,
  const std::array<byte,8> targetDevice, std::array<byte,8> * outgoingMuid)
{
  std::vector<byte> app_data(length);
  app_data.insert(app_data.end(), &data[0], &data[length]);
  int err = sendData(topic, app_data, targetDevice, outgoingMuid);
  return err;
}

int Duck::sendData(byte topic, std::vector<byte>& data,
  const std::array<byte,8> targetDevice, std::array<byte,8> * outgoingMuid)
{
   if (topic < reservedTopic::max_reserved) {
     logerr_ln("ERROR send data failed, topic is reserved.");
     return DUCKPACKET_ERR_TOPIC_INVALID;
   }
  if (data.size() > MAX_DATA_LENGTH) {
    logerr_ln("ERROR send data failed, message too large: %d bytes",data.size());
    return DUCKPACKET_ERR_SIZE_INVALID;
  }
  int err = txPacket->prepareForSending(&filter, targetDevice, this->getType(), topic, data);

  if (err != DUCK_ERR_NONE) {
    return err;
  }

  err = duckRadio.sendData(txPacket->getBuffer());

  CdpPacket packet = CdpPacket(txPacket->getBuffer());

  if (err == DUCK_ERR_NONE) {
    filter.bloom_add(packet.muid.data(), MUID_LENGTH);
  }

  if (!lastMessageAck) {
    loginfo_ln("Previous `lastMessageMuid` %s not acked",duckutils::toString(lastMessageMuid).c_str());
    loginfo_ln("Overwriting with: ",duckutils::toString(packet.muid).c_str());
  }

  lastMessageAck = false;
    std::copy(packet.muid.begin(),packet.muid.end(),lastMessageMuid.begin());
  assert(lastMessageMuid.size() == MUID_LENGTH);
  if (outgoingMuid != NULL) {
      std::copy(packet.muid.cbegin(),packet.muid.cend(),outgoingMuid->begin());
    assert(outgoingMuid->size() == MUID_LENGTH);
  }
  txPacket->reset();

  return err;
}

muidStatus Duck::getMuidStatus(const std::array<byte,4> & muid) const {
  if (duckutils::isEqual(muid, lastMessageMuid)) {
    if (lastMessageAck) {
      return muidStatus::acked;
    } else {
      return muidStatus::not_acked;
    }
  } else if (muid.size() != MUID_LENGTH) {
    return muidStatus::invalid;
  } else {
    return muidStatus::unrecognized;
  }
}

CdpPacket Duck::buildCdpPacket(byte topic, const std::vector<byte> data,
    const std::array<byte,8> targetDevice, const std::array<byte,8> &muid)
{
  if (data.size() > MAX_DATA_LENGTH) {
    logerr_ln("ERROR send data failed, message too large: %d bytes", data.size());
  }
  int err = txPacket->prepareForSending(&filter, targetDevice, this->getType(), topic, data);
  //txPacket unintended side effects?
  if (err != DUCK_ERR_NONE) {
    logerr_ln("prepare for sending failed. " + err);
  }
  //todo error not handled properly
  CdpPacket packet = CdpPacket(txPacket->getBuffer());
  packet.muid = muid;

  return packet;
}

// TODO: implement this using new packet format
bool Duck::reboot(void*) {
  /*
    std::string reboot = "REBOOT";
    loginfo_ln(reboot);
    DuckRadio::getInstance()->sendPayloadStandard(reboot, "boot");
    duckesp::restartDuck();
  */
  return true;
}

// TODO: implement this using new packet format
bool Duck::imAlive(void*) {
  /*
  std::string alive = "Health Quack";
  loginfo_ln(alive);
  DuckRadio::getInstance()->sendPayloadStandard(alive, "health");
  */
  return true;
}

int Duck::startReceive() {
  int err = duckRadio.startReceive();
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR Restarting Duck...");
    duckesp::restartDuck();
  }
  return err;
}

int Duck::sendPong() {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  err = txPacket->prepareForSending(&filter, ZERO_DUID, this->getType(), reservedTopic::pong, data);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR Oops! failed to build pong packet, err = %d", err);
    return err;
  }
  err = duckRadio.sendData(txPacket->getBuffer());
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR Oops! Lora sendData failed, err = %d", err);
    return err;
  }
  return err;
}

int Duck::sendPing() {
  int err = DUCK_ERR_NONE;
  std::vector<byte> data(1, 0);
  err = txPacket->prepareForSending(&filter, ZERO_DUID, this->getType(), reservedTopic::ping, data);
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR Failed to build ping packet, err = " + err);
    return err;
  }
  err = duckRadio.sendData(txPacket->getBuffer());
  if (err != DUCK_ERR_NONE) {
    logerr_ln("ERROR Lora sendData failed, err = %d", err);
  }
  return err;
}

bool Duck::isWifiConnected() {
  return duckNet->isWifiConnected();
}

std::string Duck::getSsid() {
  return duckNet->getSsid();
}

std::string Duck::getPassword() {
  return duckNet->getPassword();
}



std::string Duck::getErrorString(int error) {
  std::string errorStr = std::to_string(error) + ": ";

  switch (error) {
    case DUCK_ERR_NONE:
      return errorStr + "No error";
    case DUCK_ERR_NOT_SUPPORTED:
      return errorStr + "Feature not supported";
    case DUCK_ERR_SETUP:
      return errorStr + "Setup failure";
    case DUCK_ERR_ID_TOO_LONG:
      return errorStr + "Id length is invalid";
    case DUCKLORA_ERR_BEGIN:
      return errorStr + "Lora module initialization failed";
    case DUCKLORA_ERR_SETUP:
      return errorStr + "Lora module configuration failed";
    case DUCKLORA_ERR_RECEIVE:
      return errorStr + "Lora module failed to read data";
    case DUCKLORA_ERR_TIMEOUT:
      return errorStr + "Lora module timed out";
    case DUCKLORA_ERR_TRANSMIT:
      return errorStr + "Lora moduled failed to send data";
    case DUCKLORA_ERR_HANDLE_PACKET:
      return errorStr + "Lora moduled failed to handle RX data";
    case DUCKLORA_ERR_MSG_TOO_LARGE:
      return errorStr + "Attempted to send a message larger than 256 bytes";

    case DUCKWIFI_ERR_NOT_AVAILABLE:
      return errorStr + "Wifi network is not availble";
    case DUCKWIFI_ERR_DISCONNECTED:
      return errorStr + "Wifi is disconnected";
    case DUCKWIFI_ERR_AP_CONFIG:
      return errorStr + "Wifi configuration failed";

    case DUCKDNS_ERR_STARTING:
      return errorStr + "DNS initialization failed";

    case DUCKPACKET_ERR_SIZE_INVALID:
      return errorStr + "Duck packet size is invalid";
    case DUCKPACKET_ERR_TOPIC_INVALID:
      return errorStr + "Duck packet topic field is invalid";
    case DUCKPACKET_ERR_MAX_HOPS:
      return errorStr + "Duck packet reached maximum allowed hops";

    case DUCK_INTERNET_ERR_SETUP:
      return errorStr + "Internet setup failed";
    case DUCK_INTERNET_ERR_SSID:
      return errorStr + "Internet SSID is not valid";
    case DUCK_INTERNET_ERR_CONNECT:
      return errorStr + "Internet connection failed";
  }
  
  return "Unknown error";
}