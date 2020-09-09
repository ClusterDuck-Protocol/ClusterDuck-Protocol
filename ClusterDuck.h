#ifndef CD
#define CD

#include "cdpcfg.h"

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <WString.h>

#include <OTAPage.h>
#include <Update.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include "timer.h"

#include <ArduinoOTA.h>

#include "DuckDisplay.h"
#include "DuckLed.h"
#include "DuckLora.h"
#include "DuckNet.h"
#include "DuckUtils.h"

class ClusterDuck {
public:
  ClusterDuck();
  ~ClusterDuck() {
    delete _duckDisplay;
    delete _duckLed;
  };

  // Public APIs

  // Common Duck
  //-------------------------------------------------------------------------
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
  //-------------------------------------------------------------------------
  void setupDuckLink();
  void runDuckLink();

  // Mama Duck
  //-------------------------------------------------------------------------
  void setupMamaDuck();
  void runMamaDuck();

  // Detect Duck
  //-------------------------------------------------------------------------
  void setupDetect();
  int runDetect();

  // LoRa
  //-------------------------------------------------------------------------
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
  //-------------------------------------------------------------------------
  void setupDisplay(String deviceType);

  // Network
  //-------------------------------------------------------------------------
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
  //-------------------------------------------------------------------------
  void setupOTA();

  // Portal
  //-------------------------------------------------------------------------
  void processPortalRequest();
  String* getPortalDataArray();
  String getPortalDataString();
  bool runCaptivePortal();

  // Led
  //-------------------------------------------------------------------------
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