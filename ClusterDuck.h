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

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "index.h"

#include <OTAPage.h>
#include <Update.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include "timer.h"

#include <ArduinoOTA.h>

#include "DuckDisplay.h"
#include "DuckLed.h"
#include "DuckLora.h"

/*
typedef struct
{
	String senderId;
	String topic;
	String messageId;
	String payload;
	String path;
} Packet;
*/

class ClusterDuck {
public:

	ClusterDuck();

	// Public APIs

	// Common Duck
	//-------------------------------------------------------------------------
	static void setDeviceId(String deviceId = "");
	static void begin(int baudRate = CDPCFG_SERIAL_BAUD);
	static String duckMac(boolean format);
	static String uuidCreator();
	static String getDeviceId();
	static bool imAlive(void *);
	volatile bool getInterrupt();
	void flipFlag();
	void flipInterrupt();
	static void ping();

	// Duck Link
	//-------------------------------------------------------------------------
	static void setupDuckLink();
	static void runDuckLink();

	// Mama Duck
	//-------------------------------------------------------------------------
	static void setupMamaDuck();
	static void runMamaDuck();

	// Detect Duck
	//-------------------------------------------------------------------------
	static void setupDetect();
	static int runDetect();

	// LoRa
	//-------------------------------------------------------------------------
	static void setupLoRa(long BAND = CDPCFG_RF_LORA_FREQ,
			int SS = CDPCFG_PIN_LORA_CS, int RST = CDPCFG_PIN_LORA_RST,
			int DI0 = CDPCFG_PIN_LORA_DIO0, int DI1 = CDPCFG_PIN_LORA_DIO1, int TxPower = CDPCFG_RF_LORA_TXPOW);
	static int handlePacket();
	static String getPacketData(int pSize);
	static void sendPayloadStandard(
			String msg = "", String topic = "",
			String senderId = "", String messageId = "",
			String path = "");
	static Packet getLastPacket();
	static void couple(byte byteCode, String outgoing); //?! Not public
	static bool idInPath(String path); // ?! Not public
	volatile bool getFlag(); // To check if LoRa packet was received
	static void startReceive();
	static void startTransmit();
	static int getRSSI();

	// Display
	//-------------------------------------------------------------------------
	static void setupDisplay(String deviceType);

	// Network
	//-------------------------------------------------------------------------
	static void setupWebServer(bool createCaptivePortal = false);
	static void setupWifiAp(const char *AP = " 🆘 DUCK EMERGENCY PORTAL");
	static void setupDns();
	static void setupInternet(String SSID, String PASSWORD);
	static bool ssidAvailable(String val = "");
	static void setSSID(String val);
	static void setPassword(String val);
	static String getSSID();
	static String getPassword();

	// OTA - Firmware upgrade
	//-------------------------------------------------------------------------
	static void setupOTA();

	// Portal
	//-------------------------------------------------------------------------
	static void processPortalRequest();
	static String * getPortalDataArray();
	static String getPortalDataString();
	static bool runCaptivePortal();

	// Led
	//-------------------------------------------------------------------------
	static void setColor(int ledR = CDPCFG_PIN_RGBLED_R, int ledG = CDPCFG_PIN_RGBLED_G, int ledB = CDPCFG_PIN_RGBLED_B);
	static void setupLED();

protected:
	static Packet _lastPacket;
	static String _deviceId;
	static DuckDisplay _duckDisplay;
	static DuckLed _duckLed;
	static DuckLora _duckLora;
	

private:

	static void setFlag(void);
	static void restartDuck();
	static bool reboot(void *);


	static int _rssi;
	static float _snr;
	static long _freqErr;
	static int _availableBytes;

	static int _packetSize;
	static DNSServer dnsServer;
	static const byte DNS_PORT;
	static const char *DNS;
	static const char *AP;
	static String portal;

	static String runTime;

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
