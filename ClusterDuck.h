#ifndef CD
#define CD

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include "index.h"
#include "timer.h"

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LoRa.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

typedef struct {
    String senderId;
    String messageId;
    String payload;
    String path;
} Packet;

class CaptiveRequestHandler : public AsyncWebHandler
{
  public:
    CaptiveRequestHandler(String portal)
    {
        _portal = portal;
    }
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
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

    int _rssi;           ///< TODO
    float _snr;          ///< TODO
    long _freqErr;       ///< TODO
    int _availableBytes; ///< TODO

    int _packetSize; ///< TODO

    DNSServer dnsServer; ///< TODO
    const byte DNS_PORT; ///< TODO
    const char *DNS;     ///< TODO
    const char *AP;      ///< TODO
    String portal;       ///< TODO

    String runTime; ///< TODO

    // QuackPack
    static byte messageId_B; ///< TODO
    static byte payload_B;   ///< TODO
    static byte path_B;      ///< TODO

    U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8; ///< TODO

    IPAddress apIP;           ///< TODO
    AsyncWebServer webServer; ///< TODO

  protected:
    static bool reboot(void *);
    static void restart();
    static bool imAlive(void *);
    void setupLoRa(long BAND = 915E6, int SS = 18, int RST = 14, int DI0 = 26, int TxPower = 20);
    void setupDisplay(String deviceType);
    void setupPortal(const char *AP = " 🆘 DUCK EMERGENCY PORTAL");
    bool runCaptivePortal();

    void processPortalRequest();

    String *getPortalDataArray();
    String getPortalDataString();
    String *getPacketData(int pSize);
    String duckMac(boolean format);
    void sendPayloadStandard(String msg, String senderId = "", String messageId = "", String path = "");
    static String uuidCreator();
    String getDeviceId();
    Packet getLastPacket();
    void loRaReceive();
    static void couple(byte byteCode, String outgoing);

    static String _deviceId; ///< TODO
    byte ping_B;             ///< TODO
    static byte senderId_B;  ///< TODO
    byte iamhere_B;          ///< TODO

    Packet _lastPacket; ///< TODO
    Timer<> tymer;      ///< TODO

  public:
    DuckLink(/* args */);
    ~DuckLink();

    // Exposed Methods
    void setDeviceId(String deviceId = "");
    void begin(int baudRate = 115200);
    virtual void setup();
    virtual void run();
    static void sendPayloadMessage(String msg);
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

    String m_ssid;       ///< TODO
    String m_password;   ///< TODO
    String m_org;        ///< TODO
    String m_deviceId;   ///< TODO
    String m_deviceType; ///< TODO
    String m_token;      ///< TODO
    String m_server;     ///< TODO
    String m_topic;      ///< TODO
    String m_authMethod; ///< TODO
    String m_clientId;   ///< TODO
    Timer<> m_timer;     ///< TODO

    WiFiClientSecure m_wifiClient; ///< TODO
    PubSubClient m_pubSubClient;   ///< TODO

    byte m_ping; ///< TODO

  public:
    PapaDuck(String ssid, String password, String org, String deviceId, String deviceType, String token, String server,
             String topic, String authMethod, String clientId);
    ~PapaDuck();

    virtual void setup();
    virtual void run();
};

#endif
