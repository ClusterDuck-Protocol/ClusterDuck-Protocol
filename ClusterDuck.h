#ifndef CD
#define CD

#if (ARDUINO >=100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <U8x8lib.h>

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "index.h"

#include "timer.h"

// forward declaration
class PubSubClient;
class WiFiClientSecure;

typedef struct
{
  String senderId;
  String messageId;
  String payload;
  String path;
} Packet;


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

class DuckLink
{
private:

    String readMessages(byte mLength);

    static String _deviceId;
    int _rssi;
    float _snr;
    long _freqErr;
    int _availableBytes;
    
    int _packetSize;

    DNSServer dnsServer;
    const byte DNS_PORT;
    const char *DNS;
    const char *AP;
    String portal;

    String runTime;

    // QuackPack
    static byte messageId_B;
    static byte payload_B;
    static byte path_B;

    U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8;

    IPAddress apIP;
    AsyncWebServer webServer;
    

protected:

    static bool reboot(void *);
    static void restart();
    static bool imAlive(void *);
    void setupLoRa(long BAND = 915E6, int SS = 18, int RST = 14, int DI0 = 26, int TxPower = 20);
    void setupDisplay(String deviceType);
    void setupPortal(const char *AP = " 🆘 DUCK EMERGENCY PORTAL");
    bool runCaptivePortal();
    
    void processPortalRequest();
    
    String * getPortalDataArray();
    String getPortalDataString();
    String * getPacketData(int pSize);
    String duckMac(boolean format);
    void sendPayloadStandard(String msg, String senderId = "", String messageId = "", String path = "");
    static String uuidCreator();
    String getDeviceId();
    Packet getLastPacket();
    static void sendPayloadMessage(String msg);
    void loRaReceive();
    static void couple(byte byteCode, String outgoing);

    byte ping_B;
    static byte senderId_B;
    byte iamhere_B;

    Packet _lastPacket;
    Timer<> tymer;

public:
    DuckLink(/* args */);
    ~DuckLink();

    //Exposed Methods
    void setDeviceId(String deviceId = "");
    void begin(int baudRate = 115200);
    virtual void setup();
    virtual void run();


};

class MamaDuck : public DuckLink
{
private:
    bool idInPath(String path);
public:
  MamaDuck(/* args */);
  ~MamaDuck();

  virtual void setup();
  virtual void run();

};

class PapaDuck : public MamaDuck
{
private:

  void setupWiFi();
  void setupMQTT();
  void quackJson();

  String m_ssid;
  String m_password;
  String m_org;
  String m_deviceId;
  String m_deviceType;
  String m_token;
  String m_server;
  String m_topic;
  String m_authMethod;
  String m_clientId;
  Timer<>  m_timer; // create a timer with default settings

  std::unique_ptr<WiFiClientSecure> m_wifiClient;
  std::unique_ptr<PubSubClient> m_pubSubClient;

  byte m_ping;

public:
  PapaDuck(String ssid, String password, String org, String deviceId, String deviceType, String token, String server, String topic, String authMethod, String clientId);
  ~PapaDuck();

  virtual void setup();
  virtual void run();
  
};


#endif
