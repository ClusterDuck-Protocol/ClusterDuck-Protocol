/**
 * @file ClusterDuck.h
 * @author 
 * @brief This file defines the cluster duck public APIs.
 * 
 * @version 
 * @date 2020-09-15
 * 
 * @copyright
 * 
 */
#ifndef CD
#define CD

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <WString.h>

#include "include/DuckDisplay.h"
#include "include/DuckLed.h"
#include "include/DuckLora.h"
#include "include/DuckNet.h"
#include "include/DuckUtils.h"
#include "include/cdpcfg.h"
#include "timer.h"

/**
 * @brief External APIs to build and control a duck device.
 * 
 * This class exposes all the necessary APIs to setup/build a duck as well as
 * customizig the duck's behavior.
 */
class ClusterDuck {
public:
  /**
   * @brief Construct a new Cluster Duck object.
   * 
   */
  ClusterDuck();

  ~ClusterDuck() {
    delete _duckDisplay;
    delete _duckLed;
    delete _duckLora;
    delete _duckNet;
  };
  
  /**
   * @brief A Builder interface through which a Duck can be setup.
   * 
   */
  friend class DuckBuilder;

  /**
   * @brief Set the device id of a Duck.
   * 
   * @param deviceId a string representing the device id (optional: if none is provided, a random value is generated) by the library
   */
  void setDeviceId(String deviceId = "");

  /**
   * @brief Initialize the duck.
   *
   * @param baudRate the serial interface baud rate. Default value is
   * set to CDPCFG_SERIAL_BAUD
   */
  void begin(int baudRate = CDPCFG_SERIAL_BAUD);

  /**
   * @brief Get the Duck Mac Address.
   *
   * @param format    true if the mac address is formated as MM:MM:MM:SS:SS:SS
   * @return A string representing the duck mac address
   */
  String duckMac(boolean format);

  /**
   * @brief Create a uuid string.
   *
   * @return A string representing a unique identifier.
   */
  String uuidCreator();
  /**
   * @brief Get the duck's device id.
   * 
   * @return A string representing the duck's device id 
   */
  String getDeviceId();

  static bool imAlive(void*);
  static bool reboot(void*);

  /**
   * @brief Get the interrupt state.
   * 
   * @return true if interrupt is enabled, false otherwise.
   */
  volatile bool getInterrupt();

  /**
   * @brief Toggle the flag that indicates a message is received.
   * 
   */
  void flipFlag();

  /**
   * @brief Toggle the interrupt state flag.
   * 
   */
  void flipInterrupt();

  /**
   * @brief Transmit a ping message.
   *
   * @return 0 if the message was sent sucessfully, an error code otherwise.
   */
  int ping();

  /**
   * @brief Shortcut to setup a Duck Link.
   *
   * This function will setup a Duck Link and is equivalent to call these
   * individual methods: 
   * ```
   * setupDisplay("Duck");
   * setupLoRa();
   * setupWifiAp();
   * setupDns();
   * setupWebServer(true);
   * setupOTA();
   * ```
   * It is assumed that the DuckLink hardware has a display and a wifi component.
   */
  void setupDuckLink();
  /**
   * @brief starts the DuckLink run thread.
   * 
   * - listens for OTA messages and performs OTA update if needed
   * - listens for portal messages and trasmits them.
   */
  void runDuckLink();

  /**
   * @brief Shortcut to setup a MamaDuck.
   *
   * This function will setup a Mama Duck and is equivalent to call these
   * individual methods:
   * ```
   * setupDisplay("Duck");
   * setupLoRa();
   * setupWifiAp();
   * setupDns();
   * setupWebServer(true);
   * setupOTA();
   * ```
   * It is assumed that the MamaDuck hardware has a display and a wifi component.
   */
  void setupMamaDuck();

  /**
   * @brief starts the Mama Duck run thread.
   *
   * - listens for OTA messages and performs OTA update if needed
   * - listens for portal messages and trasmits them.
   * - checks for received message and relay them if needed
   */
  void runMamaDuck();

  /**
   * @brief Shortcut to setup a Duck Detector
   *
   * This function will setup a Duck Detector and is equivalent to call these
   * individual methods:
   * ```
   * setupDisplay("Duck");
   * setupLoRa();
   * setupOTA()
   * ```
   * It is assumed that the Duck Detector hardware has a display and a wifi
   * component
   */
  void setupDetect();

  /**
   * @brief starts the Duck Detector run thread.
   *
   * - checks for "health" message received and returns the LoRa RSSI value
   * @return an integer value representing the LoRa RSSI value
   */
  int runDetect();

  /**
   * @brief Setup the Lora module (default values are defined in cdpcfg.h)
   *
   * @param BAND    radio frequency (default CDPCFG_RF_LORA_FREQ
   * e.g 915.0 forthe US)
   * @param SS      slave select pin (default CDPCFG_PIN_LORA_CS)
   * @param RST     chip reset pin. (default CDPCFG_PIN_LORA_RST)
   * @param DI0     dio0 interrupt pin (default CDPCFG_PIN_LORA_DIO0)
   * @param DI1     dio1 interrupt pin (default CDPCFG_PIN_LORA_DIO1)
   * @param TxPower transmit power (default CDPCFG_RF_LORA_TXPOW)
   */
  void setupLoRa(float BAND = CDPCFG_RF_LORA_FREQ, int SS = CDPCFG_PIN_LORA_CS,
                 int RST = CDPCFG_PIN_LORA_RST, int DI0 = CDPCFG_PIN_LORA_DIO0,
                 int DI1 = CDPCFG_PIN_LORA_DIO1,
                 int TxPower = CDPCFG_RF_LORA_TXPOW);

  /**
   * @brief Handles a received LoRa packet.
   *
   * @return The packet length which should be > 0. An error code is returned if there was a failure to read the data
   * from the LoRa driver.
   */
  int handlePacket();

  /**
   * @brief Get the Packet data.
   * 
   * @param pSize the length of the packet
   * @return A string representing the content of the packet 
   */
  String getPacketData(int pSize);

  /**
   * @brief Sends a Duck LoRa message.
   * 
   * @param msg         the message payload (optional: if not provided, it will be set to an empty string)
   * @param topic       the message topic (optional: if not provided, it will be set to "status")
   * @param senderId    the sender id (optional: if not provided, it will be set to the duck device id) 
   * @param messageId   the message id (optional: if not provided, a unique id will be generated)
   * @param path        the message path to append the device id to (optional: if not provided, the path will only contain the duck's device id) 
   */
  void sendPayloadStandard(String msg="", String topic = "",
                           String senderId = "", String messageId = "",
                           String path = "");

  /** 
   * @brief Get the last received LoRa packet.
   * 
   * @return A Packet object containing the last received message.
   */
  Packet getLastPacket();

  /**
   * @brief  Append a chunk to the packet.
   *
   * A chunk is part of the Duck LoRa packet and is formated as
   * [tag][length][payload] where a tag is a single byte identifying the chunk.
   * Currently supported tags:
   * ```
   * ping
   * sender_id
   * topic
   * message_id
   * payload
   * iamhere
   * ```
   * @param byteCode identifies the tag to append
   * @param outgoing the payload for the tag to be appended to the LoRa packet
   */
  void couple(byte byteCode, String outgoing);

  /**
   * @brief Determine if a Duck device_id is present in the path.
   *
   * @param path  path retrieved from the LoRa packet
   * @return true if the id is in the path, false otherwise.
   */
  bool idInPath(String path);   

  /**
   * @brief Get the packet receive flag status
   * 
   * @return true if a received packet is available, false otherwise. 
   */
  volatile bool getFlag();

  /**
   * @brief Set the Duck to be ready to recieve LoRa packets.
   *
   * @return 0 if the call was successful, an error code otherwise.
   */
  int startReceive();

  /**
   * @brief Set the Duck to be ready to transmit LoRa packets.
   *
   * @return 0 if the call was successful, an error code otherwise.
   */
  int startTransmit();

  /**
   * @brief Get the current RSSI value.
   *
   * @return An integer representing the rssi value.
   */
  int getRSSI();

  /**
   * @brief Initialize the display component.
   *
   * @param deviceType a string representing the device type (e.g "Papa")
   */
  void setupDisplay(String deviceType);

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
   * @brief Set up the WiFi access point.
   *
   * @param accessPoint a string representing the access point. Default to  "🆘 DUCK EMERGENCY PORTAL"
   */
  void setupWifiAp(const char* accessPoint = "🆘 DUCK EMERGENCY PORTAL");

  /**
   * @brief Set up DNS.
   *
   */
  void setupDns();

  /**
   * @brief Set up internet access.
   *
   * @param ssid        the ssid of the WiFi network
   * @param password    password to join the network
   */
  void setupInternet(String ssid, String password);

  /**
   * @brief  Checks if the given ssid is available.
   *
   * @param val     ssid to check, default is an empty string and will use the
   * internal default ssid
   * @return true if the ssid is available, false otherwise.
   */
  bool ssidAvailable(String val = "");

  /**
   * @brief Set the WiFi network ssid.
   *
   * @param val the ssid string to set
   */
  void setSSID(String val);

  /**
   * @brief Set the WiFi password.
   *
   * @param val  the password string to set
   */
  void setPassword(String val);

  /**
   * @brief Get the WiFi network ssid.
   *
   * @return a string representing the current network ssid
   */
  String getSSID();

  /**
   * @brief Get the WiFi password ssid.
   *
   * @return a string representing the current network password
   */
  String getPassword();

  // OTA - Firmware upgrade
  void setupOTA();

  // Portal
  void processPortalRequest();
  String* getPortalDataArray();
  String getPortalDataString();
  bool runCaptivePortal();

  // Led
  void setColor(int ledR = CDPCFG_PIN_RGBLED_R, int ledG = CDPCFG_PIN_RGBLED_G,
                int ledB = CDPCFG_PIN_RGBLED_B);
  void setupLED();

protected:
  String _deviceId = "";
  DuckDisplay* _duckDisplay = DuckDisplay::getInstance();
  DuckLed* _duckLed = DuckLed::getInstance();

  DuckLora* _duckLora = DuckLora::getInstance();
  DuckNet* _duckNet = DuckNet::getInstance();

private:
  static void setFlag(void);
  //static void restartDuck();

  static int _rssi;
  static float _snr;
  static long _freqErr;
  static int _availableBytes;
  static int _packetSize;
  static String runTime;

  static void handleOta();
};

/**
 * @brief External duck builder APIs to setup a duck device
 * 
 */
class DuckBuilder {
public:
  /**
   * @brief Construct a new Duck Builder object
   * 
   */
  DuckBuilder() {
  }

  /**
   * @brief Initialize and setup a basic duck
   * 
   * Initialize a duck and set its device id, but does not setup any other components
   * such as the LoRa module, a display, etc...
   * 
   * @param id a string representing the duck's unique id
   * @return An updated reference to a DuckBuilder object.
   */
  auto createAs(String id) -> DuckBuilder& {
    _duck.begin();
    _duck.setDeviceId(id);
    return *this;
  }

  /**
   * @brief Initialize and setup a Mama Duck.
   *
   * The duck is initialized as a Mama Duck and will perform the following setups.
   * If the duck hardware does not have WiFi or a Display, these 
   * setups (or related setups) will be skipped.
   * ```
   * setupDisplay("Mama");
   * setupLoRa();
   * setupWifiAp();
   * setupDns();
   * setupWebServer(true);
   * setupOTA();
   * ```
   *
   * @param id a string representing the duck's unique id
   * @return An updated reference to a DuckBuilder object.
   */
  auto createMamaDuckAs(String id) -> ClusterDuck& {
    _duck.begin();
    _duck.setDeviceId(id);
    _duck.setupMamaDuck();
    return _duck;
  }

  /**
   * @brief Initialize and setup a Duck Link.
   *
   * The duck is initialized as a DuckLink and will perform the following
   * setups. If the duck hardware does not have WiFi or a Display, these
   * setups (or related setups) will be skipped.
   * ```
   * setupDisplay("Duck");
   * setupLoRa();
   * setupWifiAp();
   * setupDns();
   * setupWebServer(true);
   * setupOTA();
   * ```
   *
   * @param id a string representing the duck's unique id
   * @return An updated reference to a DuckBuilder object.
   */
  auto createDuckLinkAs(String id) -> ClusterDuck& {
    _duck.begin();
    _duck.setDeviceId(id);
    _duck.setupDuckLink();
    return _duck;
  }

  /**
   * @brief Initialize and setup a Duck Detector.
   *
   * The duck is initialized as a DuckDetector and will perform the following
   * setups. If the duck hardware does not have WiFi or a Display, these
   * setups (or related setups) will be skipped.
   * ```
   * setupDisplay("Duck");
   * setupLoRa();
   * setupWifiAp();
   * setupDns();
   * setupWebServer(true);
   * setupOTA();
   * ```
   *
   * @param id a string representing the duck's unique id
   * @return An updated reference to a DuckBuilder object.
   */
  auto createDectorDuckAs(String id) -> ClusterDuck& {
    _duck.begin();
    _duck.setDeviceId(id);
    _duck.setupDetect();
    return _duck;
  }

  /**
   * @brief Setup LoRa configuration
   *
   * @param BAND      LoRa module frequency band, (optional) default: CDPCFG_RF_LORA_FREQ
   * @param SS        LoRa module slave select pin, (optional) default: CDPCFG_PIN_LORA_CS
   * @param RST       LoRa module reset pin, (optional) default: CDPCFG_PIN_LORA_RST
   * @param DI0       LoRa module dio0 interrupt pin, (optional) default: CDPCFG_PIN_LORA_DIO0
   * @param DI1       LoRa module dio1 interrupt pin, (optional) default: CDPCFG_PIN_LORA_DIO1
   * @param TxPower   LoRa module transmit power, (optional) default: CDPCFG_RF_LORA_TXPOW
   * @return An updated reference to a DuckBuilder object.
   */
  auto withLora(long BAND = CDPCFG_RF_LORA_FREQ, int SS = CDPCFG_PIN_LORA_CS,
                int RST = CDPCFG_PIN_LORA_RST, int DI0 = CDPCFG_PIN_LORA_DIO0,
                int DI1 = CDPCFG_PIN_LORA_DIO1,
                int TxPower = CDPCFG_RF_LORA_TXPOW) -> DuckBuilder& {
    _duck.setupLoRa(BAND, SS, RST, DI0, DI1, TxPower);
    return *this;
  }

  /**
   * @brief Setup Display configuration
   *
   * This method does nothing if display is disabled in the `cdpcfg.h`
   * configuration file
   * 
   * @param deviceType  A string representing the device type (e.g "Mama",
   * "Duck",...)
   * @return An updated reference to a DuckBuilder object.
   */
  auto withDisplay(String deviceType) -> DuckBuilder& {
    _duck.setupDisplay(deviceType);
    return *this;
  }

  /**
   * @brief Setup WiFi configuration.
   *
   * This method does nothing if there is WiFi is disabled in the `cdpcfg.h`
   * configuration file.
   *
   * @param ap a string representing the wifi access point.
   * @return An updated reference to a DuckBuilder object.
   */
  auto withWifi(const char* ap = "🆘 DUCK EMERGENCY PORTAL") -> DuckBuilder& {
    _duck.setupWifiAp(ap);
    return *this;
  }

  /**
   * @brief Setup DNS configuration.
   *
   * This method does nothing if there is WiFi is disabled in the `cdpcfg.h`
   * configuration file.
   *
   * @return An updated reference to a DuckBuilder object.
   */
  auto withDns() -> DuckBuilder& {
    _duck.setupDns();
    return *this;
  }

  /**
   * @brief Setup internet configuration.
   *
   * This method does nothing if there is WiFi is disabled in the `cdpcfg.h`
   * configuration file.
   *
   * @param ssid      ssid of the wifi network
   * @param password  password to access the wifi network
   * @return An updated reference to a DuckBuilder object.
   */
  auto withInternet(String ssid, String password) -> DuckBuilder& {
    _duck.setupInternet(ssid, password);
    return *this;
  }

  /**
   * @brief Setup Over The Air (OTA) update.
   *
   * This method does nothing if there is WiFi is disabled in the `cdpcfg.h`
   * configuration file.
   * 
   * @return An updated reference to a DuckBuilder object.
   */
  auto withOta() -> DuckBuilder& {
    _duck.setupOTA();
    return *this;
  }

  /**
   * @brief
   *
   * This method does nothing if there is WiFi is disabled in the `cdpcfg.h`
   * configuration file.
   *
   * @param createCaptivePortal true if the captive portal is used, (optional:
   * default is false)
   * @param html  a string that represents the portal HTML page, (optional: if
   * not provided, will use a default webpage)
   * @return An updated reference to a DuckBuilder object.
   */
  auto withWebServer(bool createCaptivePortal = false, String html = "") -> DuckBuilder& {
    _duck.setupWebServer(createCaptivePortal, html);
    return *this;
  }

  /**
   * @brief Setup onboard LED.
   *
   * @return An updated reference to a DuckBuilder object.
   */
  auto withLed()->DuckBuilder& {
    _duck.setupLED();
    return *this;
  }

  /**
   * @brief Tell the builder the setup is complete
   * 
   * @return A setup duck ready to go.
   */
  auto done() -> ClusterDuck& { return _duck; }

private:
  ClusterDuck _duck;
  
};

#ifndef CDPCFG_WIFI_NONE

  class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler(String portal) { _portal = portal; }
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest* request) { return true; }

    void handleRequest(AsyncWebServerRequest* request) {
      AsyncResponseStream* response = request->beginResponseStream("text/html");
      response->print(_portal);
      request->send(response);
    }

    String _portal;
};
#endif

#endif