#ifndef CD
#define CD

#include "include/cdpcfg.h"

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <WString.h>

#include <Update.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <include/OTAPage.h>

#include "timer.h"

#include <ArduinoOTA.h>

#include "include/DuckDisplay.h"
#include "include/DuckLed.h"
#include "include/DuckLora.h"
#include "include/DuckNet.h"
#include "include/DuckUtils.h"

class ClusterDuck {
public:
  ClusterDuck();
  ~ClusterDuck() {
    delete _duckDisplay;
    delete _duckLed;
    delete _duckLora;
    delete _duckNet;
  };
  
  friend class DuckBuilder;

  // Public APIs
  
  // Common Duck
  void setDeviceId(String deviceId = "");
  void begin(int baudRate = CDPCFG_SERIAL_BAUD);
  String duckMac(boolean format);
  String uuidCreator();
  String getDeviceId();
  static bool imAlive(void*);
  static bool reboot(void*);

  volatile bool getInterrupt();
  void flipFlag();
  void flipInterrupt();
  void ping();

  // Duck Link
  void setupDuckLink();
  void runDuckLink();

  // Mama Duck
  void setupMamaDuck();
  void runMamaDuck();

  // Detect Duck
  void setupDetect();
  int runDetect();

  // LoRa
  void setupLoRa(long BAND = CDPCFG_RF_LORA_FREQ, int SS = CDPCFG_PIN_LORA_CS,
                 int RST = CDPCFG_PIN_LORA_RST, int DI0 = CDPCFG_PIN_LORA_DIO0,
                 int DI1 = CDPCFG_PIN_LORA_DIO1,
                 int TxPower = CDPCFG_RF_LORA_TXPOW);
  int handlePacket();
  String getPacketData(int pSize);
  void sendPayloadStandard(String msg = "", String topic = "",
                           String senderId = "", String messageId = "",
                           String path = "");
  Packet getLastPacket();
  void couple(byte byteCode, String outgoing); //?! Not public
  bool idInPath(String path);                  // ?! Not public
  volatile bool getFlag(); // To check if LoRa packet was received
  void startReceive();
  void startTransmit();
  int getRSSI();

  // Display
  void setupDisplay(String deviceType);

  // Network
  void setupWebServer(bool createCaptivePortal = false);
  void setupWifiAp(const char* AP = " 🆘 DUCK EMERGENCY PORTAL");
  void setupDns();
  void setupInternet(String SSID, String PASSWORD);
  bool ssidAvailable(String val = "");
  void setSSID(String val);
  void setPassword(String val);
  String getSSID();
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
  static Packet _lastPacket;
  String _deviceId = "";
  DuckDisplay* _duckDisplay = DuckDisplay::getInstance();
  DuckLed* _duckLed = DuckLed::getInstance();

  DuckLora* _duckLora = DuckLora::getInstance();
  DuckNet* _duckNet = DuckNet::getInstance();

private:
  static void setFlag(void);
  static void restartDuck();

  static int _rssi;
  static float _snr;
  static long _freqErr;
  static int _availableBytes;

  static int _packetSize;
  static DNSServer dnsServer;
  static const byte DNS_PORT;
  static const char* DNS;
  static const char* AP;
  static String portal;

  static String runTime;
};

class DuckBuilder {
public:
  DuckBuilder(){
  }

  auto createAs(String id) -> DuckBuilder& {
    _duck.begin();
    _duck.setDeviceId(id);
    return *this;
  }

  auto createMamaDuckAs(String id) -> ClusterDuck& {
    _duck.begin();
    _duck.setDeviceId(id);
    _duck.setupMamaDuck();
    return _duck;
  }
  auto createDuckLinkAs(String id) -> ClusterDuck& {
    _duck.begin();
    _duck.setDeviceId(id);
    _duck.setupDuckLink();
    return _duck;
  }

  auto createDectorDuckAs(String id) -> ClusterDuck& {
    _duck.begin();
    _duck.setDeviceId(id);
    _duck.setupDetect();
    return _duck;
  }
  auto withLora(long BAND = CDPCFG_RF_LORA_FREQ, int SS = CDPCFG_PIN_LORA_CS,
                int RST = CDPCFG_PIN_LORA_RST, int DI0 = CDPCFG_PIN_LORA_DIO0,
                int DI1 = CDPCFG_PIN_LORA_DIO1,
                int TxPower = CDPCFG_RF_LORA_TXPOW) -> DuckBuilder& {
    _duck.setupLoRa(BAND, SS, RST, DI0, DI1, TxPower);
    return *this;
  }

  auto withDisplay(String deviceType) -> DuckBuilder& {
    _duck.setupDisplay(deviceType);
    return *this;
  }

  auto withWifi(const char* ap = " 🆘 DUCK EMERGENCY PORTAL") -> DuckBuilder& {
    _duck.setupWifiAp(ap);
    return *this;
  }

  auto withDns() -> DuckBuilder& {
    _duck.setupDns();
    return *this;
  }

  auto withInternet(String ssid, String password) -> DuckBuilder& {
    _duck.setupInternet(ssid, password);
    return *this;
  }

  auto withOta() -> DuckBuilder& {
    _duck.setupOTA();
    return *this;
  }

  auto withWebServer(bool createCaptivePortal = false) -> DuckBuilder& {
    _duck.setupWebServer(createCaptivePortal);
    return *this;
  }
  auto withLed()->DuckBuilder& {
    _duck.setupLED();
    return *this;
  }
  auto done() -> ClusterDuck& { return _duck; }

private:
  ClusterDuck _duck;
  };

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