#ifndef CD
#define CD

#include "cdpcfg.h"

#if (ARDUINO >=100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <RadioLib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <U8x8lib.h>

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "index.h"

#include "timer.h"

#include <ArduinoOTA.h>

typedef struct
{
  String senderId;
  String messageId;
  String payload;
  String path;
} Packet;

class ClusterDuck {
  public:
    //Constructor
    ClusterDuck();

    //Exposed Methods
    static void setDeviceId(String deviceId = "");
    static void begin(int baudRate = CDPCFG_SERIAL_BAUD);
    static void setupLoRa(long BAND = CDPCFG_RF_LORA_FREQ, int SS = CDPCFG_PIN_LORA_CS, int RST = CDPCFG_PIN_LORA_RST, int DI0 = CDPCFG_PIN_LORA_DIO0, int DI1 = CDPCFG_PIN_LORA_DIO1, int TxPower = CDPCFG_RF_LORA_TXPOW);
    static void setupDisplay(String deviceType);
    static void setupWebServer(bool createCaptivePortal = false);
		static void setupWifiAp(const char *AP = " 🆘 DUCK EMERGENCY PORTAL");
		static void setupDns();
		static void setupInternet(String SSID, String PASSWORD);
    static void setupOTA();
    static bool runCaptivePortal();

    static void setupDuckLink();
    static void setupMamaDuck();
    static void processPortalRequest();
    static int handlePacket();
    static void runDuckLink();
    static void runMamaDuck();

    static String * getPortalDataArray();
    static String getPortalDataString();
    static String getPacketData(int pSize);

    static String duckMac(boolean format);

    static int _rssi;
    static float _snr;
    static long _freqErr;
    static int _availableBytes;

    static void sendPayloadStandard(String msg, String senderId = "", String messageId = "", String path = "");

    static String uuidCreator();

    static String getDeviceId();
    static Packet getLastPacket();

    static void sendPayloadMessage(String msg);
    static bool imAlive(void *);

    static void couple(byte byteCode, String outgoing);
    static bool idInPath(String path);

    volatile bool getFlag();
    volatile bool getInterrupt();
    void flipFlag();
    void flipInterrupt();
    static void startReceive();
    static int getRSSI();
    static void ping();
    static void startTransmit();
    static void setupDetect();
    static int runDetect();

    static void setColor(int ledR = CDPCFG_PIN_RGBLED_R, int ledG = CDPCFG_PIN_RGBLED_G, int ledB = CDPCFG_PIN_RGBLED_B);
    static void setupLED();

    static String getSSID();
    static String getPassword();

  protected:
    static Packet _lastPacket;
    static String _deviceId;

  private:

    static int _packetSize;

    static void setFlag(void);

    static DNSServer dnsServer;
    static const byte DNS_PORT;
    static const char *DNS;
    static const char *AP;
    static String portal;

    static String runTime;

    static void restartDuck();
    static bool reboot(void *);

    // QuackPack
    static byte ping_B;
    static byte senderId_B;
    static byte messageId_B;
    static byte payload_B;
    static byte iamhere_B;
    static byte path_B;

    static int ledR;
    static int ledG;
    static int ledB;


};

class CaptiveRequestHandler: public AsyncWebHandler {
  public:
    CaptiveRequestHandler(String portal) {
      _portal = portal;
    }
    virtual ~CaptiveRequestHandler() { }

    bool canHandle(AsyncWebServerRequest *request) {
      return true;
    }

    void handleRequest(AsyncWebServerRequest *request) {
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      response->print(_portal);
      request->send(response);
    }

    String _portal;
};

#endif
