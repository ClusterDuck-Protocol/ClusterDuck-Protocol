#include "DuckUtils.h"
#include <iomanip>
#include <sstream>
#include <string>

#include "DuckLogger.h"

namespace duckutils {

Timer<> duckTimer = timer_create_default();
bool detectState = false;

std::string getCDPVersion() {
  return std::to_string(CDP_VERSION_MAJOR) + "." + std::to_string(CDP_VERSION_MINOR) + "." + std::to_string(CDP_VERSION_PATCH);
}

Timer<> getTimer() { return duckTimer; }

bool getDetectState() { return detectState; }
bool flipDetectState() {
  detectState = !detectState;
  return detectState;

}

void  getRandomBytes(int length, byte* bytes) {
  const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;
  for (i = 0; i < length; i++) {
    //TODO: Random generator here isn't seeded properly
    //We can use RSSI value to seed it or use a frame counter if available
    bytes[i] = digits[random(36)];    
  }
}

std::string createUuid(int length) {
  std::string msg = "";
  int i;

  for (i = 0; i < length; i++) {
    byte randomValue = random(36);
    if (randomValue < 26) {
      msg = msg + char(randomValue + 'a');
    } else {
      msg = msg + char((randomValue - 26) + '0');
    }
  }
  return msg;
}

// Note: This function is not thread safe
std::string convertToHex( byte* data, int size) {
  std::string buf = ""; // static to avoid memory leak
  buf.clear();
  buf.reserve(size * 2); // 2 digit hex
  const char* cs = "0123456789ABCDEF";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0F];
    buf += cs[val & 0x0F];
  }
  return buf;
}

uint32_t toUint32(const byte* data) {
    uint32_t value = 0;

    value |= data[0] << 24;
    value |= data[1] << 16;
    value |= data[2] << 8;
    value |= data[3];
    return value;
}

#ifdef CDPCFG_WIFI_NONE
int saveWifiCredentials(std::string ssid, std::string password) {
  logwarn_ln("WARNING saveWifiCredentials skipped, device has no WiFi.");
  return DUCK_ERR_NOT_SUPPORTED;
}

std::string loadWiFiPassword() {
  logwarn_ln("WARNING loadWiFiPassword skipped, device has no WiFi.");
  return "unknown";
}
std::string loadWifiSsid() {
  logwarn_ln("WARNING loadWifiSsid skipped, device has no WiFi.");
  return "unknown";
}

#else
int saveWifiCredentials(std::string ssid, std::string password) {
  int err = DUCK_ERR_NONE;

  if (ssid.empty() || password.empty()) {
    logerr("Invalid SSID or password\n");
    return DUCK_ERR_INVALID_ARGUMENT;
  }
  if (!EEPROM.begin(512)) {
    logerr("Failed to initialise EEPROM\n");
    return DUCK_ERR_EEPROM_INIT;
  }

  if (ssid.length() > 0 && password.length() > 0) {
    loginfo("Clearing EEPROM\n");
    for (int i = 0; i < 96; i++) {
      EEPROM.write(i, 0);
    }

    loginfo("updating EEPROM...\n");
    for (int i = 0; i < ssid.length(); i++)
    {
      EEPROM.write(i, ssid[i]);
    }
    for (int i = 0; i < password.length(); ++i)
    {
      EEPROM.write(32 + i, password[i]);
    }
    if (!EEPROM.commit()) {
      logerr("Failed to commit EEPROM\n");
      err = DUCK_ERR_EEPROM_WRITE;
    }
  }
  return err;
}

std::string loadWifiSsid() {
  EEPROM.begin(512); //Initialasing EEPROM
  std::string esid;
  // loop through saved SSID characters
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  return esid;
}

std::string loadWifiPassword() {
  EEPROM.begin(512); //Initialasing EEPROM
  std::string epass = "";
  // loop through saved Password characters
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  return epass;
}
#endif

std::string toUpperCase(std::string str) {
  std::string upper = "";
  for (int i = 0; i < str.length(); i++) {
    upper += toupper(str[i]);
  }
  return upper;
}

  // Note: This function is provided as a convenience for Arduino users who are using String in their code
  // This function should not be used in CDP library code!
  std::vector<uint8_t> stringToByteVector(const std::string& str) {
      std::vector<uint8_t> byteVec;
      byteVec.reserve(str.length());

      for (unsigned int i = 0; i < str.length(); ++i) {
          byteVec.push_back(static_cast<byte>(str[i]));
      }

      return byteVec;
  }

  /**
   * @brief Get an error code description.
   * 
   * @param error an error code
   * @returns a string describing the error. 
   */
  std::string getErrorString(int error) {//seems more like a duck logger or utils type of thing
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

} // namespace duckutils
