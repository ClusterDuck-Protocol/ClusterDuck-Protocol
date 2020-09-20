#ifndef DUCK_H
#define DUCK_H

#include <Arduino.h>
#include <WString.h>

#include "cdpcfg.h"

#include "DuckLora.h"
#include "DuckNet.h"

class Duck {

public:
  Duck(){}
  Duck(String id, int baudRate = 115200);
  ~Duck() {
  }

  void setupSerial(int baudRate = 115200);
  void setupRadio(float band = CDPCFG_RF_LORA_FREQ, int ss = CDPCFG_PIN_LORA_CS,
                  int rst = CDPCFG_PIN_LORA_RST, int di0 = CDPCFG_PIN_LORA_DIO0,
                  int di1 = CDPCFG_PIN_LORA_DIO1,
                  int txPower = CDPCFG_RF_LORA_TXPOW);

  /**
   * @brief Set up the WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to  
   * "🆘 DUCK EMERGENCY PORTAL"
   */
  void setupWifi(const char* ap = "🆘 DUCK EMERGENCY PORTAL");

  /**
   * @brief Set up DNS.
   *
   */
  void setupDns();
  /**
   * @brief Set up the WebServer.
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
  void setupWebServer(bool createCaptivePortal = false, String html = "");

  /**
   * @brief Set up internet access.
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   */
  void setupInternet(String ssid, String password);
  void setupOTA();

  /**
   * @brief Sends a Duck LoRa message.
   *
   * @param msg         the message payload (optional: if not provided, it will
   * be set to an empty string)
   * @param topic       the message topic (optional: if not provided, it will be
   * set to "status")
   * @param senderId    the sender id (optional: if not provided, it will be set
   * to the duck device id)
   * @param messageId   the message id (optional: if not provided, a unique id
   * will be generated)
   * @param path        the message path to append the device id to (optional:
   * if not provided, the path will only contain the duck's device id)
   * @returns 0 if success, an error code otherwise
   */

  int sendPayloadStandard(String msg = "", String topic = "",
                           String senderId = "", String messageId = "",
                           String path = "");

protected:

  String deviceId;
  DuckLora* duckLora = DuckLora::getInstance();
  DuckNet* duckNet = DuckNet::getInstance();

  int startReceive();
  int startTransmit();

  virtual int run() = 0;
  virtual void setupWithDefaults() {
    duckNet->setDeviceId(deviceId);
  }

  static volatile bool receivedFlag;
  void toggleReceiveFlag() { receivedFlag = !receivedFlag; }

  /**
   * @brief Interrupt service routing when LoRa chip di0 pin is active
   * 
   */
  static void onPacketReceived();

  static bool imAlive(void*);
  static bool reboot(void*);

  void processPortalRequest();
  void handleOtaUpdate();
};

#endif