#ifndef DUCK_H
#define DUCK_H

#include "../DuckError.h"
#include "DuckNet.h"
#include "DuckRadio.h"
#include "DuckTypes.h"
#include "DuckPacket.h"
#include "cdpcfg.h"
#include <Arduino.h>
#include <WString.h>
#include <string>

class Duck {

public:
  /**
   * @brief Construct a new Duck object.
   *
   */
  Duck(String name="");

  virtual ~Duck() {
    if (txPacket != NULL) {
      delete txPacket;
    }
    if (rxPacket != NULL) {
      delete rxPacket;
    }
  }

  /**
   * @brief Set the Device Name object
   * 
   * @param name 
   */
  void setName(String name) { this->duckName = name; }
  
  /**
   * @brief Get the duck's name.
   * 
   * @returns A string representing the duck's name
   */
  String getName() {return duckName;}
  /**
   * @brief setup the duck unique ID
   * 
   * @param an 8 byte unique id 
   * @return DUCK_ERR_NONE if successful, an error code otherwise 
   */
  int setDeviceId(std::vector<byte> id);

  /**
   * @brief setup the duck unique ID
   *
   * @param an 8 byte unique id
   * @return DUCK_ERR_NONE if successful, an error code otherwise
   */
  int setDeviceId(byte* id);

  /**
   * @brief Setup serial connection.
   *
   * @param baudRate default: 115200
   */
  int setupSerial(int baudRate = 115200);

  /**
   * @brief Setup the radio component
   *
   * @param band      radio frequency in Mhz (default: 915.0)
   * @param ss        slave select pin (default CDPCFG_PIN_LORA_CS)
   * @param rst       reset pin  (default: CDPCFG_PIN_LORA_RST)
   * @param di0       dio0 interrupt pin (default: CDPCFG_PIN_LORA_DIO0)
   * @param di1       dio1 interrupt pin (default: CDPCFG_PIN_LORA_DIO1)
   * @param txPower   transmit power (default: CDPCFG_RF_LORA_TXPOW)
   */
  int setupRadio(float band = CDPCFG_RF_LORA_FREQ, int ss = CDPCFG_PIN_LORA_CS,
                  int rst = CDPCFG_PIN_LORA_RST, int di0 = CDPCFG_PIN_LORA_DIO0,
                  int di1 = CDPCFG_PIN_LORA_DIO1,
                  int txPower = CDPCFG_RF_LORA_TXPOW);

  /**
   * @brief Setup WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to
   * "🆘 DUCK EMERGENCY PORTAL"
   *
   * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
   */
  int setupWifi(const char* ap = "🆘 DUCK EMERGENCY PORTAL");

  /**
   * @brief Setup DNS.
   *
   * @returns DUCK_ERROR_NONE if successful, an error code otherwise.
   */
  int setupDns();

  /**
   * @brief Setup web server.
   *
   * The WebServer is used to communicate with the Duck over ad-hoc WiFi
   * connection.
   *
   * @param createCaptivePortal set to true if Captive WiFi connection is
   * needed. Defaults to false
   * @param html A string representing custom HTML code used for the portal.
   * Default is an empty string Default portal web page is used if the string is
   * empty
   */
  int setupWebServer(bool createCaptivePortal = false, String html = "");

  /**
   * @brief Setup internet access.
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   */
  int setupInternet(String ssid, String password);

  /**
   * @brief
   *
   */
  int setupOTA();

  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a string representing the data
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @return DUCK_ERR_NONE if the data was send successfully, an error code otherwise. 
   */
  int sendData(byte topic, const String data, const std::vector<byte> targetDevice = ZERO_DUID);

  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a vector of bytes representing the data to send
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @return DUCK_ERR_NONE if the data was send successfully, an error code
   otherwise.
   */
  int sendData(byte topic, std::vector<byte> bytes, const std::vector<byte> targetDevice = ZERO_DUID);

  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a string representing the data to send
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @return DUCK_ERR_NONE if the data was send successfully, an error code
   * otherwise.
   */
  int sendData(byte topic, const std::string data, const std::vector<byte> targetDevice = ZERO_DUID);

  /**
   * @brief Sends data into the mesh network.
   *
   * @param topic the message topic
   * @param data a byte buffer representing the data to send
   * @param length the length of the byte buffer
   * @param targetDevice the device UID to receive the message (default is no target device)
   * @return DUCK_ERR_NONE if the data was send successfully, an error code
   * otherwise.
   */
  int sendData(byte topic, const byte* data, int length, const std::vector<byte> targetDevice = ZERO_DUID);

  /**
   * @brief Check wifi connection status
   * 
   * @returns true if device wifi is connected, false otherwise. 
   */
  bool isWifiConnected() { return duckNet->isWifiConnected(); }
  /**
   * @brief Check if the give access point is available.
   * 
   * @param ssid access point to check
   * @returns true if the access point is available, false otherwise.
   */
  bool ssidAvailable(String ssid) { return duckNet->ssidAvailable(ssid); }

  /**
   * @brief Get the access point ssid
   * 
   * @returns the wifi access point as a string
   */
  String getSsid() { return duckNet->getSsid(); }
  /**
   * @brief Get the wifi access point password.
   * 
   * @returns the wifi access point password as a string. 
   */
  String getPassword() { return duckNet->getPassword(); }

  /**
   * @brief Get an error code description.
   * 
   * @param error an error code
   * @returns a string describing the error. 
   */
  String getErrorString(int error);

protected:
  String duckName="";

  String deviceId;
  std::vector<byte> duid;
  DuckRadio* duckRadio = DuckRadio::getInstance();
  DuckNet* duckNet = DuckNet::getInstance();
  DuckPacket* txPacket = NULL;
  DuckPacket* rxPacket = NULL;

  /**
   * @brief sends a pong message
   * 
   * @return DUCK_ERR_NONE if successfull. An error code otherwise 
   */
  int sendPong();
  
  /**
   * @brief sends a ping message
   *
   * @return DUCK_ERR_NONE if successfull. An error code otherwise
   */
  int sendPing();

  /**
   * @brief Tell the duck radio to start receiving packets from the mesh network
   *
   * @return DUCK_ERR_NONE if successful, an error code otherwise
   */
  int startReceive();

  /**
   * @brief Implement the duck's specific behavior.
   * 
   * This method must be implemented by the Duck's concrete classes such as DuckLink, MamaDuck,...
   */
  virtual void run() = 0;

  /**
   * @brief Setup a duck with default settings
   *
   * The default implementation simply initializes the serial interface.
   * It can be overriden by each concrete Duck class implementation.
   */
  virtual int setupWithDefaults(std::vector<byte> deviceId, String ssid, String password) {
    int err = setupSerial();
    if (err != DUCK_ERR_NONE) {
      return err;
    }
    err = setDeviceId(deviceId);
    if (err != DUCK_ERR_NONE) {
      return err;
    }
    return DUCK_ERR_NONE;
  }

  /**
   * @brief Get the duck type.
   * 
   * @returns A value representing a DuckType
   */
  virtual int getType() = 0;

  /**
   * @brief reconnect the duck to the given wifi access point
   * 
   * @param ssid the access point ssid to connect to 
   * @param password the access point password
   * @return DUCK_ERR_NONE if the duck reconnected to the AP sucessfully. An error code otherwise. 
   */
  virtual int reconnectWifi(String ssid, String password) {
    return DUCK_ERR_NONE;
  }

  /**
   * @brief Handle request from emergency portal.
   *
   */
  void processPortalRequest();

  /**
   * @brief Handle over the air firmware update.
   *
   */
  void handleOtaUpdate();

  static volatile bool receivedFlag;
  static void setReceiveFlag(bool value) { receivedFlag = value; }
  static bool getReceiveFlag() { return receivedFlag; }

  static void onRadioRxTxDone();

  static bool imAlive(void*);
  static bool reboot(void*);
};

#endif
