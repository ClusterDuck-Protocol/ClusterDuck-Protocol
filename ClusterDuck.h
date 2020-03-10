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
#include <WebServer.h>
#include <ESPmDNS.h>
#include "index.h"

#include "timer.h"

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
    static void setDeviceId(String deviceId = "", const int formLength = 10);
    static void begin(int baudRate = 115200);
    static void setupLoRa(long BAND = 915E6, int SS = 18, int RST = 14, int DI0 = 26, int TxPower = 20);
    static void setupDisplay(String deviceType);
    static void setupPortal(const char *AP = " 🆘 DUCK EMERGENCY PORTAL");
    static bool runCaptivePortal();

    static void setupDuckLink();
    static void setupMamaDuck();
    static void runDuckLink();
    static void runMamaDuck();

    static String * getPortalDataArray();
    static String getPortalDataString();
    static String * getPacketData(int pSize);

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

    static void loRaReceive();

    static void couple(byte byteCode, String outgoing);
    static bool idInPath(String path);

  protected:
    static Packet _lastPacket;
    static String _deviceId;

  private:

    static int _packetSize;

    static DNSServer dnsServer;
    static const byte DNS_PORT;
    static const char *DNS;
    static const char *AP;
    static String portal;

    static String runTime;

    static void restartDuck();
    static String readMessages(byte mLength);
    static bool reboot(void *);

    static String * formArray;
    static int fLength;

    // QuackPack
    static byte ping_B;
    static byte senderId_B;
    static byte messageId_B;
    static byte payload_B;
    static byte iamhere_B;
    static byte path_B;


};

#endif
