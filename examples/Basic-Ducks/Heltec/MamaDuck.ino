/**
 * @file MamaDuck.ino
 * @brief Implements a MamaDuck using the ClusterDuck Protocol (CDP).
 *
 * This example firmware periodically sends sensor health data (counter and free memory)
 * through a CDP mesh network. It also relays messages that it receives from other ducks
 * that has not seen yet.
 *
 * @date 2025-05-07
 */

 #include <string>
 #include <vector>
 #include <cstdio>
 #include <cstring>
 #include <cstdlib>
 #include <cmath>
 #include <map>
 #include <arduino-timer.h>
 #include <CDP.h>
 #include "payloads/DuckPayloads.h"
 #ifdef SERIAL_PORT_USBVIRTUAL
 #define Serial SERIAL_PORT_USBVIRTUAL
 #endif
 #include <heltec_unofficial.h>
// #include "wifi.h"
 #include "image.h"
 #include "Lang.h"
 #include <NimBLEDevice.h>

// Access the RadioLib radio instance from DuckLoRa.cpp to read RSSI/SNR.
// getSignalScore() is protected in Duck, so we reach the object directly.
extern CDPCFG_LORA_CLASS lora;

// ── Heltec V4 GPS support (L76K on UART1, Wireless Tracker pinout) ────────────
// If your V4 variant uses different pins, adjust the defines below.
#ifdef ARDUINO_heltec_wifi_lora_32_V4
  #include <TinyGPSPlus.h>
  #define GPS_RX_PIN    39    // ESP32 RX ← GPS TX
  #define GPS_TX_PIN    38    // ESP32 TX → GPS RX
  #define GPS_BAUD      9600
  #define GPS_VGNSS_PIN  34   // pull LOW to power the GPS module
  static TinyGPSPlus    tinyGps;
  static HardwareSerial gpsSerial(1);  // UART1
  static bool gpsModuleDetected = false;
  static bool gpsFix             = false;
#endif

#ifdef ARDUINO_heltec_wifi_lora_32_V4
#define LORA_PA_POWER  7
#define LORA_PA_EN     2
#endif

 // ── Device Identification ────────────────────────────────────────────────────
 // Duck ID: MUST be exactly 8 bytes and unique on the mesh.
 // To pin a fixed, human-readable ID, `#define DUCK_ID "MYDUCK01"` above this
 // line (exactly 8 characters). If DUCK_ID is left undefined, one is
 // auto-derived below from this board's factory-unique WiFi MAC/efuse address
 // (see duckesp::getDuckMacAddress()), so every device gets a distinct,
 // reboot-stable ID with no manual configuration required.
 static char DUCK_ID_BUF[9] = {0};
 static bool initDuckId() {
 #ifdef DUCK_ID
   strncpy(DUCK_ID_BUF, DUCK_ID, 8);
 #else
   std::string mac = duckesp::getDuckMacAddress(false);  // unformatted hex, e.g. "E4B4C2A1B2C3"
   std::string id  = (mac.length() >= 8) ? mac.substr(mac.length() - 8) : std::string("DUCK0000");
   memcpy(DUCK_ID_BUF, id.c_str(), 8);
 #endif
   DUCK_ID_BUF[8] = '\0';
   return true;
 }
 static bool duckIdReady = initDuckId();
 #define DUCK_NAME DUCK_ID_BUF   // kept so the rest of this sketch is unchanged
 // Bluetooth Low energgy definitions
 #define NUS_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
 #define NUS_RX_CHAR "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
 #define NUS_TX_CHAR "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

 
 // --- Function Declarations ---
 bool runSensor(void *);
 bool sendEmergency(String lat = "", String lng = "", String alt = "", String spd = "", String hdg = "", bool gpsFromPhone = false);
 void handleDuckData(CdpPacket packet);
 void displayMessage(String msg);
 void displayAnnouncement(const String& msg);
 void flashLED();
 void displayID();
 void displayBatt();
 void displayHome();
 void handleFrame(const String& line);
 void broadcast(const String& frame);
 void sendBattery();
 void handleSOS(const String& body);
 void handleMsg(const String& body);
 void handleMamaTalk(const String& body);
 bool sendMamaTalk(const String& targetId, const String& msg, const String& mid = "");
 String extractField(const String& body, const String& key);
 void sendFrame(const String& frame);
 void handleGps(const String& body);
 static float readVbat();
 void blinkLed(int frequency);
 static void quickBlink();
 void showHoldProgress(uint32_t heldMs);
 static bool isPhoneConnected();

 // --- Global Variables ---
 MamaDuck duck(DUCK_NAME); // Device ID, MUST be 8 bytes and unique from other ducks;
 auto timer = timer_create_default();  // Creating a timer with default settings
 const int INTERVAL_MS = 10000;        // Interval in milliseconds between runSensor call
 int counter = 1;                      // Counter for the sensor data  
 bool setupOK = false;                 // Flag to check if setup is complete
 char buffer[100];
 const char* s = DUCK_NAME;

static NimBLECharacteristic* pTxChar    = nullptr;
static NimBLEServer*          pBleServer = nullptr;  // stored for updateConnParams
static bool bleConnected = false;
static String bleInBuf = "";
static String usbInBuf = "";
static unsigned long lastUsbRxMs = 0;   // last time a byte arrived over USB
static bool usbPhoneSeen = false;        // true once phone sends any CDK frame via USB
static bool          bleAdvertising    = true;   // track advertising state
static unsigned long bleAdvSlowAfterMs = 0;      // millis() deadline; switch to slow adv after this
static bool displayEnabled = true;       // track display on/off state
const unsigned long USB_IDLE_TIMEOUT_MS  = 30000UL;  // resume BLE after 30 s of USB silence
const unsigned long GPS_PHONE_TIMEOUT_MS = 300000UL; // treat USB phone as gone after 5 min of silence
static unsigned long lastBattMs = 0;
static unsigned long lastUsbTxMs = 0;    // last time we wrote a frame to USB serial

// ── Phone GPS display (written from BLE task, rendered in main loop) ──────
// display.xxx uses I2C which is not thread-safe; BLE callbacks run on the
// NimBLE task so we must defer all display calls to the main loop.
static volatile bool phoneGpsDisplayPending  = false;
static volatile bool phoneGpsNoFix           = false;
static volatile bool gpsLoraOk               = false;  // result of last duck.sendData() for GPS
static volatile bool gpsTxPending            = false;  // deferred GPS LoRa TX requested by handleGps()
static volatile bool bleConnectDisplayPending = false; // show BLE-connected splash from main loop
static volatile bool bleDisconnectDisplayPending = false; // show BLE-disconnected splash from main loop
static volatile bool usbConnectDisplayPending = false; // show USB-connected splash from main loop
static volatile bool usbDisconnectDisplayPending = false; // show USB-disconnected splash from main loop
static std::vector<uint8_t> gpsTxPayload;              // encoded protobuf payload for deferred GPS TX
static char          phoneGpsLatBuf[20]      = {};
static char          phoneGpsLngBuf[20]     = {};
static char          phoneGpsAltBuf[12]     = {};     // altitude in metres
static char          phoneGpsSpdBuf[12]     = {};     // speed in km/h
static char          phoneGpsHdgBuf[12]     = {};     // heading in degrees
static unsigned long gpsReqSentMs           = 0;      // millis() when CDK:GPSREQ was sent; 0 if none pending
static unsigned long gpsReqDeferredSendMs   = 0;      // millis() at which the main loop should send CDK:GPSREQ
static unsigned long gpsDisplayClearMs      = 0;      // millis() at which the main loop should call display.displayOff()
static int           lastSignalPct           = -1;     // last LoRa signal quality (0-100 %), -1 = no packet yet
static unsigned long lastHomeRefreshMs       = 0;      // last time displayHome() was refreshed from the loop
const  unsigned long HOME_REFRESH_MS         = 5000UL; // re-draw home screen every 5 s when display is on
static bool          messagePending          = false;  // true while a received message occupies the screen
static unsigned long messagePendingMs        = 0;      // millis() when message was shown (for auto-dismiss)
const  unsigned long MESSAGE_DISPLAY_MS      = 10000UL; // auto-dismiss received message after 10 s
static bool          emergencyDisplayPending = false;  // emergency screen — stays until program button pressed
static int           lastTxResult            = -1;     // -1=never sent, 0=success, >0=fail
static unsigned long lastTxMs               = 0;      // millis() of most recent duck.sendData() call
static volatile bool sosAckDisplayPending   = false;  // set by handleDuckData; rendered at top of loop()
static volatile unsigned long bleAnnounceAfterMs = 0; // millis() after which to send ID+battery post-connect
static unsigned long          bleSplashClearMs   = 0; // millis() after which to call displayHome() after connect splash

// ── SOS state (non-blocking) ─────────────────────────────────────────────────
// Set when the 2s-hold has fired but there's no cached phone GPS yet, so we
// broadcast CDK:GPSREQ and wait (asynchronously) instead of blocking loop()
// for up to 2.5 s. A single click while this is true cancels the pending SOS.
static bool           sosPending        = false;
static unsigned long  sosWaitStartMs    = 0;
const  unsigned long  SOS_GPS_WAIT_MS   = 2500UL;
const  uint32_t       SOS_HOLD_MS       = 2000UL;   // matches button.pressedFor(2000) below

// ── Per-duck GPS cache ────────────────────────────────────────────────────────
// Populated whenever a packet containing LAT:/LNG: fields arrives from any duck.
// Entries are valid for 5 minutes (same TTL as the app's Nearby Ducks list).
struct DuckGps { float lat; float lng; unsigned long tsMs; };
static std::map<String, DuckGps> duckGpsCache;  // key: 8-char duck ID
constexpr unsigned long DUCK_GPS_TTL_MS = 300000UL;  // 5 minutes

// ── Custom discovery topics ───────────────────────────────────────────────────
// BEACON (27) replaces the PING + separate GPS packet pair with a single packet
// that embeds the sender's GPS inline.  The receiver replies with BEACON_ACK (28)
// which also embeds its GPS.  Both sides get GPS-rich CDK:SEEN from the very first
// received packet — no GPSREQ chain, no case-234 blocking delay, 2 TX not 4.
static const uint8_t TOPIC_BEACON     = 27;
static const uint8_t TOPIC_BEACON_ACK = 28;
static volatile bool beaconAckPending = false;   // deferred BEACON_ACK TX
static char          beaconAckPayload[80] = {};   // GPS payload for beacon ACK
static unsigned long beaconAckDeferMs = 0;        // millis() after which ACK may be sent

// ── BLE callbacks ─────────────────────────────────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleConnected = true;
    bleAnnounceAfterMs = millis() + 700;
    bleConnectDisplayPending = true;
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo& connInfo, int reason) override {
    bleConnected = false;
    bleInBuf = "";
    bleDisconnectDisplayPending = true;  // notify main loop to render splash
    // Only restart advertising if USB is not currently active
    bool usbActive = (lastUsbRxMs > 0 && millis() - lastUsbRxMs < USB_IDLE_TIMEOUT_MS);
    if (!usbActive) {
      // Fast advertising for 30 s after disconnect to help the phone re-discover quickly,
      // then fall back to slow (200–400 ms) to ease 2.4 GHz congestion.
      NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
      pAdv->setMinInterval(160);  // 100 ms — fast
      pAdv->setMaxInterval(160);
      bleAdvSlowAfterMs = millis() + 30000;
      NimBLEDevice::startAdvertising();
      bleAdvertising = true;
    }
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    std::string val = pChar->getValue();
    for (char c : val) {
      if (c == '\n') { handleFrame(bleInBuf); bleInBuf = ""; }
      else if (c != '\r') bleInBuf += c;
    }
  }
};

 /**
  * @brief Setup function to initialize the MamaDuck
  *
  * - Sets up the Duck device ID (exactly 8 bytes).
  * - Initializes MamaDuck using default configuration.
  * - Sets up periodic execution of sensor data transmissions.
  */
 void setup() {
   heltec_setup(); 
   heltec_ve(true);

   // Init display.
   std::snprintf(buffer, sizeof(buffer), "ID:%s", s);
   display.init();
   display.flipScreenVertically();
   display.clear();
   display.display();

   // BLE init first — must run regardless of duck setup outcome
   // so the device is always discoverable.
   NimBLEDevice::init(DUCK_NAME);
   NimBLEDevice::setPower(ESP_PWR_LVL_P9);
   // Use a random static address — Android 13 privacy mode on some OEMs
   // refuses connections from peripherals with a static public address.
   NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
   // Offer MTU 512 so Android 13 MTU exchange succeeds without fragmentation.
   NimBLEDevice::setMTU(512);
   // No bonding/MITM — open access for maximum disaster-scenario accessibility.
   NimBLEDevice::setSecurityAuth(false, false, false);
   NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
   pBleServer = NimBLEDevice::createServer();
   pBleServer->setCallbacks(new ServerCallbacks());
   NimBLEService* pSvc = pBleServer->createService(NUS_SERVICE);
   pTxChar = pSvc->createCharacteristic(NUS_TX_CHAR, NIMBLE_PROPERTY::NOTIFY);
   NimBLECharacteristic* pRxChar = pSvc->createCharacteristic(
     NUS_RX_CHAR, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
   pRxChar->setCallbacks(new RxCallbacks());
   pSvc->start();
   {
     NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
     pAdv->setMinInterval(160);
     pAdv->setMaxInterval(160);
     // Primary advert: flags + name (iOS CoreBluetooth discovers by name).
     NimBLEAdvertisementData advData;
     advData.setFlags(0x06);  // LE General Discoverable | BR/EDR Not Supported
     advData.setName(DUCK_NAME);
     pAdv->setAdvertisementData(advData);
     // Scan response: NUS service UUID so Android 13 validates the service
     // during scanning and doesn't drop the connection immediately after linking.
     NimBLEAdvertisementData scanRsp;
     scanRsp.addServiceUUID(NUS_SERVICE);
     pAdv->setScanResponseData(scanRsp);
     bool bleOk = NimBLEDevice::startAdvertising();
     bleAdvSlowAfterMs = millis() + 30000;
     Serial.println(bleOk ? "[BLE] Advertising started OK" : "[BLE] startAdvertising FAILED");
     if (!bleOk) {
       // Show error briefly so the operator knows BLE failed to start.
       display.clear();
       display.setFont(ArialMT_Plain_10);
       display.setTextAlignment(TEXT_ALIGN_CENTER);
       display.drawString(64, 22, TXT_BLE_ADV_FAIL);
       display.display();
       heltec_delay(2000);
       display.clear();
       display.display();
     }
   }

   if (duck.setupWithDefaults() != DUCK_ERR_NONE) {
     Serial.println("[MAMA] Failed to setup MamaDuck");
     return;
   }
   // Without this the duck spends up to 80 s in SEARCHING state where
   // duck.run() silently discards every received packet and sendData()
   // transmits nothing. Both ducks are standalone peers — no join needed.
   duck.goPublic();
   Serial.println("[MAMA] Network state: PUBLIC");
 
   setupOK = true;
   Serial.println("[MAMA] Setup OK!");

#ifdef ARDUINO_heltec_wifi_lora_32_V4
  // Power on and begin GPS (L76K / Air530 NMEA at 9600 baud on UART1)
  pinMode(GPS_VGNSS_PIN, OUTPUT);
  digitalWrite(GPS_VGNSS_PIN, LOW);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("[GPS] V4 GPS module powered on");
#endif

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // Display already initialised at top of setup(); just show the splash.
  display.clear();
  display.flipScreenVertically();

  display.drawXbm(20, 0, taqisystems_small_width, taqisystems_small_height, taqisystems_small_bits);
  display.display();
  heltec_delay(10000);
  display.clear();

  displayHome();

  duck.onReceiveDuckData(handleDuckData);
  heltec_delay(5000);
  displayHome();

 Serial.begin(115200);
  delay(200);
#ifdef SERIAL_PORT_USBVIRTUAL
  Serial.setTxTimeoutMs(0);  // never block on TX when host hasn't opened the port yet
#endif

  // Send ID + battery over USB immediately
  Serial.println(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
  Serial.println("[MAMA] Firmware v2 (with LAT/LNG support)");
  sendBattery();

  // BLE already started at top of setup() before duck init

 }

 /**
  * @brief Main loop runs continuously.
  *
  * Executes scheduled tasks and maintains Duck operation.
  */
 void loop() {
   if (!setupOK) {
     return; 
   }

  // Render phone GPS display here (main loop) — BLE callbacks can't use I2C.
  if (phoneGpsDisplayPending) {
    phoneGpsDisplayPending = false;
    display.displayOn();
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "Batt: " + String(heltec_battery_percent(readVbat())) + "%");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 0, buffer);
    if (phoneGpsNoFix) {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, TXT_PHONE_GPS_NO_SIGNAL_2L);
      display.display();
      delay(2000);
    } else {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 14, gpsLoraOk ? TXT_GPS_SENT_OK : TXT_GPS_SEND_FAIL);
      display.drawString(0, 28, "LAT:" + String(phoneGpsLatBuf));
      display.drawString(0, 40, "LNG:" + String(phoneGpsLngBuf));
      display.drawString(0, 52, TXT_SRC_PHONE);
      display.display();
      delay(3000);
    }
    display.displayOff();
  }

  // BLE connected splash — deferred from onConnect() callback.
  // Non-blocking: show splash immediately, schedule displayHome() 2 s later
  // so loop() keeps running (duck.run(), heltec_loop()) during the window.
  if (bleConnectDisplayPending) {
    bleConnectDisplayPending = false;
    display.displayOn();
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, TXT_BT_CONNECTED);
    display.display();
    bleSplashClearMs = millis() + 2000;
  }

  if (bleSplashClearMs > 0 && millis() >= bleSplashClearMs) {
    bleSplashClearMs = 0;
    if (!emergencyDisplayPending && !messagePending) displayHome();
  }

  // Deferred post-connect announce — send ID + battery after Android has
  // finished MTU negotiation and service discovery (~500 ms after connect).
  // Sending these in onConnect() itself triggers GATT error 133 on Android.
  if (bleAnnounceAfterMs > 0 && millis() >= bleAnnounceAfterMs && bleConnected) {
    bleAnnounceAfterMs = 0;
    broadcast(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
    sendBattery();
    Serial.println("[BLE] Post-connect announce sent (ID + battery)");
  } else if (bleAnnounceAfterMs > 0 && !bleConnected) {
    bleAnnounceAfterMs = 0;  // cancelled — phone disconnected before window
  }

  // BLE disconnected splash — deferred from onDisconnect() callback.
  if (bleDisconnectDisplayPending) {
    bleDisconnectDisplayPending = false;
    display.displayOn();
    displayEnabled = true;
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, TXT_BT_DISCONNECTED);
    display.display();
    delay(2000);
    displayHome();
  }

  // USB serial connected splash — shown once per connect/reconnect cycle.
  if (usbConnectDisplayPending) {
    usbConnectDisplayPending = false;
    display.displayOn();
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, TXT_USB_CONNECTED);
    display.display();
    delay(2000);
    displayHome();
  }

  // USB serial disconnected splash — shown after USB_IDLE_TIMEOUT_MS of silence.
  if (usbDisconnectDisplayPending) {
    usbDisconnectDisplayPending = false;
    display.displayOn();
    displayEnabled = true;
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, TXT_USB_DISCONNECTED);
    display.display();
    delay(2000);
    displayHome();
  }

  // SOS acknowledgement from operator — deferred from handleDuckData to avoid
  // being overwritten by sendEmergency()'s post-send display sequence.
  if (sosAckDisplayPending) {
    sosAckDisplayPending = false;
    displayEnabled = true;
    display.displayOn();
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, TXT_SOS_ACK_DISPLAY);
    display.display();
    blinkLed(3);
    messagePending   = false;  // SOS ack is treated as an emergency — button-only dismiss
    emergencyDisplayPending = true;
    displayEnabled = true;
  }

  // Deferred SOS: continue once the phone's GPS reply arrives, or once the
  // wait times out — replaces the old blocking while() loop inside the
  // button.pressedFor(2000) handler so loop()/button polling never stalls.
  if (sosPending && (phoneGpsLatBuf[0] != '\0' || millis() - sosWaitStartMs >= SOS_GPS_WAIT_MS)) {
    sosPending = false;
    String gpsLat = "", gpsLng = "", gpsAlt = "", gpsSpd = "", gpsHdg = "";
    bool gpsFromPhone = false;
    if (phoneGpsLatBuf[0] != '\0') {
      gpsLat = String(phoneGpsLatBuf);
      gpsLng = String(phoneGpsLngBuf);
      if (phoneGpsAltBuf[0] != '\0') gpsAlt = String(phoneGpsAltBuf);
      if (phoneGpsSpdBuf[0] != '\0') gpsSpd = String(phoneGpsSpdBuf);
      if (phoneGpsHdgBuf[0] != '\0') gpsHdg = String(phoneGpsHdgBuf);
      gpsFromPhone = true;
      Serial.printf("[SOS] Phone GPS acquired: lat=%s lng=%s alt=%s spd=%s hdg=%s\n",
                    gpsLat.c_str(), gpsLng.c_str(), gpsAlt.c_str(), gpsSpd.c_str(), gpsHdg.c_str());
    } else {
      Serial.println("[SOS] Phone GPS timeout — SOS will be sent without coordinates");
    }
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, TXT_SENDING_SOS_2L);
    display.display();
    sendEmergency(gpsLat, gpsLng, gpsAlt, gpsSpd, gpsHdg, gpsFromPhone);
  }

   //timer.tick();

  // Auto-dismiss received message after MESSAGE_DISPLAY_MS so the home screen
  // (and signal percentage) keeps updating even if nobody presses the button.
  if (messagePending && (millis() - messagePendingMs >= MESSAGE_DISPLAY_MS)) {
    messagePending = false;
    displayEnabled = true;
    display.displayOn();
    displayHome();
  }

  // Auto-refresh the home screen so the signal percentage stays current.
  // Reads RSSI directly from the radio here so relay and RREQ packets
  // (processed internally by CDP, never reaching handleDuckData) also
  // contribute to the signal reading. Guard: rawRssi < 0 means a real
  // packet has been received; 0 is the chip default before any reception.
  // Skipped when a received message is occupying the screen (messagePending).
  if (displayEnabled
      && !phoneGpsDisplayPending
      && !bleConnectDisplayPending
      && !usbConnectDisplayPending
      && !messagePending
      && !emergencyDisplayPending
      && (millis() - lastHomeRefreshMs >= HOME_REFRESH_MS)) {
    lastHomeRefreshMs = millis();
    float rawRssi = lora.getRSSI();
    if (rawRssi < 0.0f) {
      float normRssi = constrain((rawRssi        - RSSI_MIN) / (RSSI_MAX - RSSI_MIN), 0.0f, 1.0f);
      float normSnr  = constrain((lora.getSNR()  - SNR_MIN)  / (SNR_MAX  - SNR_MIN),  0.0f, 1.0f);
      lastSignalPct  = (int)(((normRssi + normSnr) / 2.0f) * 100.0f);
    }
    displayHome();
  }

  heltec_loop();
  // Button
  // Escalating hold feedback toward the SOS threshold. HotButton's
  // pressedFor(ms) fires exactly once per distinct increasing threshold
  // during a single continuous hold, so no manual counter is needed here
  // (unlike WioTracker's hand-rolled checkButton() debounce state machine).
  {
    static bool     holdProgressShown  = false;
    static uint32_t holdPressStartMs   = 0;
    static uint32_t lastProgressDrawMs = 0;
    const  uint32_t PROGRESS_REDRAW_MS = 150UL;

    if (button.pressed()) {
      holdPressStartMs  = millis();
      holdProgressShown = false;
    }

    if (button.pressedNow()) {
      uint32_t heldMs = millis() - holdPressStartMs;
      if (heldMs >= 300 && heldMs < SOS_HOLD_MS && (millis() - lastProgressDrawMs >= PROGRESS_REDRAW_MS)) {
        showHoldProgress(heldMs);
        lastProgressDrawMs = millis();
        holdProgressShown  = true;
      }
    } else if (holdProgressShown) {
      // Released before reaching the SOS threshold — restore the home screen.
      holdProgressShown = false;
      displayHome();
    }

    if (button.pressedFor(500) || button.pressedFor(1000) || button.pressedFor(1500)) {
      quickBlink();
    }
  }

  if (button.pressedFor(2000)) {
    String gpsLat = "";
    String gpsLng = "";
    String gpsAlt = "";
    String gpsSpd = "";
    String gpsHdg = "";
    bool gotGps = false;
#ifdef ARDUINO_heltec_wifi_lora_32_V4
    if (tinyGps.location.isValid() && tinyGps.location.age() < 5000) {
      gpsLat = String(tinyGps.location.lat(), 6);
      gpsLng = String(tinyGps.location.lng(), 6);
      if (tinyGps.altitude.isValid())  gpsAlt = String(tinyGps.altitude.meters(), 1);
      if (tinyGps.speed.isValid())     gpsSpd = String(tinyGps.speed.kmph(), 1);
      if (tinyGps.course.isValid())    gpsHdg = String(tinyGps.course.deg(), 1);
      gotGps = true;
      Serial.printf("[GPS] Fix: lat=%s lng=%s alt=%sm spd=%skm/h hdg=%sdeg\n",
                    gpsLat.c_str(), gpsLng.c_str(), gpsAlt.c_str(), gpsSpd.c_str(), gpsHdg.c_str());
    } else {
      Serial.println("[GPS] No valid fix — trying phone GPS");
    }
#endif
    // If no hardware GPS and phone is connected, use cached GPS.
    // If cache is empty (e.g. GPS poll hasn't fired yet since connect),
    // send CDK:GPSREQ now and wait up to 2.5 s for the phone to reply.
    bool sendNow = true;
    if (!gotGps && isPhoneConnected()) {
      if (phoneGpsLatBuf[0] == '\0') {
        // No cached fix yet — request one and defer the actual send instead
        // of blocking here for up to 2.5 s, which used to stall button
        // polling and the rest of the mesh loop.
        displayID();
        displayBatt();
        display.setFont(ArialMT_Plain_10);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 22, TXT_REQ_GPS_FROM_PHONE_2L);
        display.display();
        broadcast("CDK:GPSREQ");
        Serial.println("[SOS] Phone GPS cache empty — requesting fresh fix before SOS...");
        sosPending     = true;
        sosWaitStartMs = millis();
        sendNow        = false;
      } else {
        gpsLat = String(phoneGpsLatBuf);
        gpsLng = String(phoneGpsLngBuf);
        if (phoneGpsAltBuf[0] != '\0') gpsAlt = String(phoneGpsAltBuf);
        if (phoneGpsSpdBuf[0] != '\0') gpsSpd = String(phoneGpsSpdBuf);
        if (phoneGpsHdgBuf[0] != '\0') gpsHdg = String(phoneGpsHdgBuf);
        Serial.printf("[SOS] Phone GPS acquired: lat=%s lng=%s alt=%s spd=%s hdg=%s\n",
                      gpsLat.c_str(), gpsLng.c_str(), gpsAlt.c_str(), gpsSpd.c_str(), gpsHdg.c_str());
      }
    } else if (!gotGps) {
      Serial.println("[GPS] No GPS available — SOS sent without coordinates");
    }

    if (sendNow) {
      displayID();
      displayBatt();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 22, TXT_SENDING_SOS_2L);
      display.display();

      sendEmergency(gpsLat, gpsLng, gpsAlt, gpsSpd, gpsHdg, /* gpsFromPhone= */ !gotGps && gpsLat.length() > 0);
    }
  }

  if (button.isSingleClick()) {
    if (sosPending) {
      // Cancel the deferred SOS wait — user tapped instead of waiting it out.
      sosPending = false;
      displayID();
      displayBatt();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 22, TXT_SOS_CANCELLED);
      display.display();
      heltec_delay(1000);
      displayHome();
    } else if (emergencyDisplayPending) {
      // Dismiss the emergency message and return to home screen.
      emergencyDisplayPending = false;
      displayEnabled = true;
      display.displayOn();
      displayHome();
    } else if (messagePending) {
      // Dismiss the current message and return to home screen.
      messagePending = false;
      displayEnabled = true;
      display.displayOn();
      displayHome();
    } else {
      displayEnabled = !displayEnabled;
      if (displayEnabled) {
        display.displayOn();
        displayHome();
      } else {
        display.displayOff();
      }
    }
  }

  // Double-click: Roger acknowledgement — moved from triple-click since
  // this is the more time-critical rescue-coordination action and deserves
  // the fewer-clicks slot. The old double-click battery-send feature was
  // removed: battery is already auto-broadcast on BLE connect and
  // periodically over USB, so a manual send added no new information.
  if (button.isDoubleClick()) {
    // Send "Roger" confirmation to the rescuer over the LoRa mesh —
    // protobuf-encoded StatusMsg wrapped in a StatusReport (same topic,
    // topics::status, as a phone-sent message).
    duckcdp_StatusMsg rogerMsg = duckcdp_StatusMsg_init_zero;
    rogerMsg.src = duckcdp_StatusMsgSrc_STATUS_MSG_SRC_DEVICE;
    std::snprintf(rogerMsg.text, sizeof(rogerMsg.text), "%s", "Roger");
    std::vector<uint8_t> rogerEncoded = duckpayload::encodeStatusReportMsg(rogerMsg);
    duck.sendData(topics::status, rogerEncoded.data(), rogerEncoded.size());
    broadcast("CDK:ACK,ID:ROGER");
    Serial.println("[MAMA] Triple-click: Roger sent");
    display.displayOn();
    displayEnabled = true;
    display.clear();
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, TXT_ROGER_SENT);
    display.display();
    heltec_delay(2000);
    displayHome();
  }

  // Triple-click: GPS/date-time pages — moved from quadruple-click now
  // that only three gestures (single/double/triple) are used.
#ifdef ARDUINO_heltec_wifi_lora_32_V4
  if (button.isTripleClick()) {
    display.displayOn();
    displayEnabled = true;
    display.clear();
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    if (tinyGps.location.isValid()) {
      // Page 1: coordinates + satellite quality
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 14, "LAT:" + String(tinyGps.location.lat(), 5));
      display.drawString(0, 26, "LNG:" + String(tinyGps.location.lng(), 5));
      display.drawString(0, 38, "SATS:" + String(tinyGps.satellites.value()));
      display.drawString(0, 50, "AGE:" + String(tinyGps.location.age()) + "ms");
      display.display();
      heltec_delay(2500);

      // Page 2: date and time (UTC+8)
      int h  = tinyGps.time.hour() + 8;
      int mi = tinyGps.time.minute();
      int sc = tinyGps.time.second();
      int d  = tinyGps.date.day();
      int mo = tinyGps.date.month();
      int y  = tinyGps.date.year();
      if (h >= 24) {
        h -= 24; d++;
        const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        uint8_t maxD = dim[mo - 1];
        if (mo == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) maxD = 29;
        if (d > maxD) { d = 1; mo++; if (mo > 12) { mo = 1; y++; } }
      }
      display.clear();
      displayID();
      displayBatt();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      if (tinyGps.date.isValid() && tinyGps.time.isValid()) {
        char dateBuf[20], timeBuf[20];
        snprintf(dateBuf, sizeof(dateBuf), "DATE:%04d/%02d/%02d", y, mo, d);
        snprintf(timeBuf, sizeof(timeBuf), "TIME:%02d:%02d:%02d", h, mi, sc);
        display.drawString(0, 14, dateBuf);
        display.drawString(0, 26, timeBuf);
        display.drawString(0, 38, "(GMT+8 / UTC+8)");
      } else {
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 28, TXT_DATETIME_NO_SIGNAL_2L);
      }
      display.display();
      heltec_delay(2500);

      Serial.printf("[GPS] Triple-click: lat=%.5f lng=%.5f sats=%u age=%lums date=%04d/%02d/%02d time=%02d:%02d:%02d\n",
                    tinyGps.location.lat(), tinyGps.location.lng(),
                    tinyGps.satellites.value(), tinyGps.location.age(),
                    y, mo, d, h, mi, sc);
    } else if (gpsModuleDetected) {
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, TXT_GPS_MODULE_ACTIVE_2L);
      Serial.println("[GPS] Triple-click: module active, no fix yet");
    } else {
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, TXT_GPS_NO_MODULE);
      Serial.println("[GPS] Triple-click: no GPS module detected");
    }
    display.display();
    heltec_delay(5000);
    displayHome();
  }
#endif

  // Periodically announce identity over USB until the phone replies with any CDK frame.
  // NOT gated on (bool)Serial — Android apps typically don't assert DTR, so (bool)Serial
  // stays false even when the port is open. setTxTimeoutMs(0) in setup() ensures writes
  // never block when no host is reading.
  {
    static unsigned long lastUsbAnnounceMs = 0;
    // Reset after USB silence so a reconnected phone gets a fresh announcement.
    if (usbPhoneSeen && lastUsbRxMs > 0 && millis() - lastUsbRxMs > USB_IDLE_TIMEOUT_MS) {
      usbPhoneSeen      = false;
      lastUsbAnnounceMs = 0;
      usbDisconnectDisplayPending = true;  // notify main loop to render splash
    }
    if (!usbPhoneSeen && millis() - lastUsbAnnounceMs >= 3000UL) {
      Serial.println(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
      // Only send battery over BLE when NOT already BLE-connected — the
      // deferred announce + periodic 60 s timer handle the BLE cadence.
      // Firing every 3 s from the USB discovery loop floods Android.
      if (!bleConnected) sendBattery();
      lastBattMs        = millis();
      lastUsbAnnounceMs = millis();
    }
  }

  // USB incoming
  while (Serial.available()) {
    lastUsbRxMs = millis();  // mark USB as active
    char c = Serial.read();
    if (c == '\n') {
      if (usbInBuf.startsWith("CDK:")) {
        if (!usbPhoneSeen) usbConnectDisplayPending = true;  // first CDK frame → show splash
        usbPhoneSeen = true;  // phone sent a valid frame
      }
      handleFrame(usbInBuf);
      usbInBuf = "";
    }
    else if (c != '\r') usbInBuf += c;
  }

  // BLE advertising suppressed while a USB serial phone session is active.
  // "Active" means the phone has sent at least one CDK: frame AND the last
  // frame arrived within USB_IDLE_TIMEOUT_MS.  Opening the serial monitor
  // alone (no CDK frames) does NOT suppress BLE.
  if (!bleConnected) {
    if (usbPhoneSeen && bleAdvertising) {
      NimBLEDevice::stopAdvertising();
      bleAdvertising = false;
    } else if (!usbPhoneSeen && !bleAdvertising) {
      NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
      pAdv->setMinInterval(160);
      pAdv->setMaxInterval(160);
      bleAdvSlowAfterMs = millis() + 30000;
      NimBLEDevice::startAdvertising();
      bleAdvertising = true;
    }
  }

  // Transition fast → slow BLE advertising after 30 s (only when active)
  if (bleAdvertising && !bleConnected && bleAdvSlowAfterMs > 0 && millis() >= bleAdvSlowAfterMs) {
    bleAdvSlowAfterMs = 0;
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    NimBLEDevice::stopAdvertising();
    pAdv->setMinInterval(320);  // 200 ms — slow to ease 2.4 GHz congestion
    pAdv->setMaxInterval(640);  // 400 ms
    NimBLEDevice::startAdvertising();
  }

  // Periodic battery update: every 10 s when BLE connected, 60 s otherwise
  if (millis() - lastBattMs >= (bleConnected ? 10000UL : 60000UL)) {
    sendBattery();
    lastBattMs = millis();
    // Reduce BLE TX power when battery is low so the LDO can still sustain
    // advertising bursts without the supply sagging and corrupting packets.
    int battPct = heltec_battery_percent(readVbat());
    // Refresh the home screen so the battery reading stays current while idle.
    if (displayEnabled) {
      displayHome();
    }
  }
  delay(5);

#ifdef ARDUINO_heltec_wifi_lora_32_V4
  // Feed NMEA sentences from GPS module into TinyGPSPlus
  static uint32_t      totalGpsBytes = 0;
  static unsigned long lastGpsDiagMs = 0;
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    totalGpsBytes++;
    if (!gpsModuleDetected) {
      gpsModuleDetected = true;
      Serial.println("[GPS] Module detected — NMEA data incoming");
    }
    tinyGps.encode(c);
  }
  // Every 5 s, print a diagnostic until the module is confirmed
  if (!gpsModuleDetected && millis() - lastGpsDiagMs >= 5000) {
    lastGpsDiagMs = millis();
    Serial.printf("[GPS] Waiting... bytes rx: %u  pin: RX=GPIO%d TX=GPIO%d  baud: %d\n",
                  totalGpsBytes, GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD);
  }
  if (!gpsFix && tinyGps.location.isValid()) {
    gpsFix = true;
    Serial.printf("[GPS] Fix acquired: lat=%.6f, lng=%.6f, sats=%u\n",
                  tinyGps.location.lat(), tinyGps.location.lng(),
                  tinyGps.satellites.value());
  }
#endif

  duck.run();

  // ── Deferred GPS LoRa TX ────────────────────────────────────────────
  // Run AFTER duck.run() so serviceInterruptFlags() has already processed the
  // stale TX_DONE from the relay — otherwise goToReceiveMode() would call
  // startReceive() and abort the GPS response startTransmit().
  if (gpsTxPending) {
    gpsTxPending = false;
    int result = duck.sendData(topics::gps, gpsTxPayload.data(), gpsTxPayload.size());
    gpsLoraOk = (result == 0);
    Serial.printf("[GPS] Deferred LoRa TX %s (%u bytes)\n", gpsLoraOk ? "OK" : "FAILED",
                  (unsigned)gpsTxPayload.size());
  }

  // ── Deferred BEACON_ACK TX ──────────────────────────────────────────
  // Two-stage defer: when beaconAckPending is set, arm a 350 ms deadline
  // so the relay TX started by forwardPacket() inside handleReceivedPacket
  // (~160 ms on air) has time to complete before we start a new TX.
  if (beaconAckPending && beaconAckDeferMs == 0) {
    beaconAckDeferMs = millis() + 350;
    Serial.println("[BEACON] ACK TX armed (350 ms relay-clear delay)");
  }
  if (beaconAckDeferMs > 0 && millis() >= beaconAckDeferMs && !gpsTxPending) {
    beaconAckDeferMs = 0;
    beaconAckPending = false;
    int result = duck.sendData(TOPIC_BEACON_ACK, std::string(beaconAckPayload), BROADCAST_DUID);
    Serial.printf("[BEACON] ACK TX %s: %s\n", result == 0 ? "OK" : "FAILED", beaconAckPayload);
  }

  // ── Deferred GPS display clear ──────────────────────────────────────────
  // Replaces the blocking delay() that was inside case 234 of handleDuckData.
  if (gpsDisplayClearMs > 0 && millis() >= gpsDisplayClearMs) {
    gpsDisplayClearMs = 0;
    display.displayOff();
  }

  // ── Deferred CDK:GPSREQ dispatch ────────────────────────────────────────
  // CDK:GPSREQ must NOT be sent as an immediate BLE follow-up to CDK:SEEN or
  // CDK:SCAN_ACK — Android's BLE stack silently drops the second of two rapid
  // back-to-back notifications.  We schedule it here, 300-400 ms after the
  // preceding notification, so both deliveries succeed.
  if (gpsReqDeferredSendMs > 0 && millis() >= gpsReqDeferredSendMs) {
    gpsReqDeferredSendMs = 0;
    if (isPhoneConnected() && phoneGpsLatBuf[0] == '\0') {
      broadcast("CDK:GPSREQ");
      gpsReqSentMs = millis();
      Serial.println("[GPS] Deferred CDK:GPSREQ sent");
    } else if (phoneGpsLatBuf[0] != '\0' && !gpsTxPending) {
      // GPS already cached from a previous GPSREQ — respond immediately
      // without a new round-trip to the phone so OpenDMS always gets an answer.
      duckcdp_GpsReading reading = duckcdp_GpsReading_init_zero;
      reading.has_fix = true;
      reading.source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
      reading.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NONE;
      reading.lat_e7 = (int32_t)lround(atof(phoneGpsLatBuf) * 1e7);
      reading.lng_e7 = (int32_t)lround(atof(phoneGpsLngBuf) * 1e7);
      if (phoneGpsAltBuf[0] != '\0') reading.alt_m = (int32_t)lround(atof(phoneGpsAltBuf));
      if (phoneGpsSpdBuf[0] != '\0') reading.spd_dkmh = (uint32_t)lround(atof(phoneGpsSpdBuf) * 10);
      if (phoneGpsHdgBuf[0] != '\0') reading.hdg_deg = (uint32_t)lround(atof(phoneGpsHdgBuf));
      reading.batt_pct = heltec_battery_percent(readVbat());
      gpsTxPayload = duckpayload::encodeGps(reading);
      gpsTxPending = true;
      Serial.printf("[GPS] Deferred GPS TX from cache: lat=%s lng=%s\n", phoneGpsLatBuf, phoneGpsLngBuf);
    } else {
      Serial.println("[GPS] Deferred CDK:GPSREQ skipped (no phone)");
    }
  }

  // ── GPS request timeout fallback ────────────────────────────────────────
  // If CDK:GPSREQ was sent but the phone never replied within 10 s, report
  // no-fix to the mesh so OpenDMS gets an answer instead of silence.
  if (gpsReqSentMs > 0 && !gpsTxPending && millis() - gpsReqSentMs > 10000UL) {
    gpsReqSentMs = 0;
    duckcdp_GpsReading noGps = duckcdp_GpsReading_init_zero;
    noGps.has_fix = false;
    noGps.source = duckcdp_GpsSource_GPS_SOURCE_NONE;
    noGps.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NO_RESPONSE;
    noGps.batt_pct = heltec_battery_percent(readVbat());
    std::vector<uint8_t> encoded = duckpayload::encodeGps(noGps);
    duck.sendData(topics::gps, encoded.data(), encoded.size());
    Serial.println("[GPS] GPSREQ timeout — no response from phone, sent no-fix report.");
  }
 }

 void handleDuckData(CdpPacket packet) {
    bool isForMe = (memcmp(packet.dduid.data(), duck.getDuckId().data(), 8) == 0);
    bool isBroadcast = (packet.dduid[0] == 0xFF);

    Serial.printf("[RX] topic=%u duckType=%u isForMe=%d isBroadcast=%d src=%.8s\n",
                  packet.topic, (uint8_t)packet.duckType, (int)isForMe, (int)isBroadcast,
                  (char*)packet.sduid.data());
    // Debug to USB serial only — broadcasting to BLE here causes a back-to-back
    // notify immediately before CDK:SEEN which Android drops reliably.
    Serial.printf("CDK:STATUS,RX_TOPIC:%u,RX_TYPE:%u\n",
                  packet.topic, (uint8_t)packet.duckType);

    // ── 1. Extract GPS from this packet and update cache ─────────────────────
    // Any packet whose data contains LAT:/LNG: fields (GPS reports, SOS, etc.)
    // is used to cache the sender's location.  This runs BEFORE CDK:SEEN so
    // the emitted frame includes GPS on the very first packet from a duck.
    {
        String pdata = String((char*)packet.data.data(), packet.data.size());
        Serial.printf("[RX-GPS] topic=%u src=%.8s payload=%.48s\n",
                      packet.topic, (char*)packet.sduid.data(), pdata.c_str());
        int latIdx = pdata.indexOf("LAT:");
        int lngIdx = pdata.indexOf("LNG:");
        if (latIdx >= 0 && lngIdx >= 0) {
            int latEnd = pdata.indexOf(',', latIdx + 4);
            int lngEnd = pdata.indexOf(',', lngIdx + 4);
            float lat = pdata.substring(latIdx + 4, latEnd < 0 ? (int)pdata.length() : latEnd).toFloat();
            float lng = pdata.substring(lngIdx + 4, lngEnd < 0 ? (int)pdata.length() : lngEnd).toFloat();
            // Skip null-island (0,0) which indicates a missing fix
            if (!(lat == 0.0f && lng == 0.0f)) {
                String sid((char*)packet.sduid.data(), 8);
                duckGpsCache[sid] = { lat, lng, millis() };
                Serial.printf("[RX-GPS] Cached GPS for %.8s: lat=%.6f lng=%.6f\n",
                              (char*)packet.sduid.data(), lat, lng);
            } else {
                Serial.println("[RX-GPS] Skipped GPS: null-island (0,0)");
            }
        } else {
            Serial.printf("[RX-GPS] No LAT/LNG in payload (topic=%u)\n", packet.topic);
        }
    }

    // ── 2. Emit CDK:SEEN for ALL overheard ducks ─────────────────────────────
    // Include GPS coordinates when cached and fresh (within DUCK_GPS_TTL_MS).
    // Emit for every duck type so the app sees PAPA, LINK, DETC etc. too.
    {
        const char* typeStr = "UNKN";
        switch ((uint8_t)packet.duckType) {
            case DuckType::MAMA:     typeStr = "MAMA"; break;
            case DuckType::LINK:     typeStr = "LINK"; break;
            case DuckType::PAPA:     typeStr = "PAPA"; break;
            case DuckType::DETECTOR: typeStr = "DETC"; break;
            default: break;
        }
        String sid((char*)packet.sduid.data(), 8);
        sid.trim();
        if (sid.length() > 0 && sid != String((char*)duck.getDuckId().data(), 8).c_str()) {
            // Don't report ourselves
            auto gpsIt = duckGpsCache.find(sid);
            if (gpsIt != duckGpsCache.end() && millis() - gpsIt->second.tsMs < DUCK_GPS_TTL_MS) {
                char seenBuf[80];
                snprintf(seenBuf, sizeof(seenBuf), "CDK:SEEN,ID:%.8s,TYPE:%s,LAT:%.6f,LNG:%.6f",
                         (char*)packet.sduid.data(), typeStr, gpsIt->second.lat, gpsIt->second.lng);
                Serial.printf("[SEEN] Emitting with GPS: %s\n", seenBuf);
                broadcast(seenBuf);
            } else {
                char seenBuf[48];
                snprintf(seenBuf, sizeof(seenBuf), "CDK:SEEN,ID:%.8s,TYPE:%s",
                         (char*)packet.sduid.data(), typeStr);
                Serial.printf("[SEEN] Emitting without GPS (cache %s): %s\n",
                              gpsIt != duckGpsCache.end() ? "expired" : "miss", seenBuf);
                broadcast(seenBuf);
            }
        } else {
            Serial.printf("[SEEN] Skipped: sid='%s' (self or empty)\n", sid.c_str());
        }
    }

    // Relay packets: SEEN already emitted above; no further processing needed.
    // Exception: BEACON / BEACON_ACK use PAPADUCK_DUID as a "all nearby mamas"
    // address so isForMe=0 and isBroadcast=0 — they must reach the switch below.
    if (!isForMe && !isBroadcast
        && packet.topic != TOPIC_BEACON && packet.topic != TOPIC_BEACON_ACK) return;

    Serial.println("HANDLING RECEIVING DATA....");

    String message = String((char*)packet.data.data(), packet.data.size());
    Serial.println("[RX] payload: " + message);

    switch (packet.topic) {
        case reservedTopic::ping: {
            // Another duck pinged us — fetch our GPS and broadcast it so the
            // pinging duck (and any listener) can cache our location.
            duckcdp_GpsReading pingGps = duckcdp_GpsReading_init_zero;
            bool haveGps = false;
            bool haveHardwareGps = false;
#ifdef ARDUINO_heltec_wifi_lora_32_V4
            // Priority 1: hardware GPS with a fresh fix.
            if (tinyGps.location.isValid() && tinyGps.location.age() < 30000) {
                pingGps.has_fix = true;
                pingGps.source = duckcdp_GpsSource_GPS_SOURCE_DEVICE;
                pingGps.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NONE;
                pingGps.lat_e7 = (int32_t)lround(tinyGps.location.lat() * 1e7);
                pingGps.lng_e7 = (int32_t)lround(tinyGps.location.lng() * 1e7);
                pingGps.batt_pct = heltec_battery_percent(readVbat());
                haveGps = true;
                haveHardwareGps = true;
            }
#endif
            if (!haveHardwareGps) {
                // Priority 2: phone GPS. Use the cache if it's already populated.
                // If the cache is empty, defer a CDK:GPSREQ to the main loop
                // (300 ms from now) so it is NOT sent as a rapid BLE follow-up
                // immediately after the CDK:SEEN emitted above — Android's BLE
                // stack silently drops the second of two back-to-back notifications,
                // which would leave phoneGpsLatBuf permanently empty on the first
                // ping after a fresh connection.
                if (phoneGpsLatBuf[0] == '\0' && isPhoneConnected()) {
                    if (gpsReqDeferredSendMs == 0) {
                        gpsReqDeferredSendMs = millis() + 300;
                    }
                    Serial.println("[PING] GPS cache empty — CDK:GPSREQ deferred to main loop");
                }
                if (phoneGpsLatBuf[0] != '\0') {
                    pingGps.has_fix = true;
                    pingGps.source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
                    pingGps.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NONE;
                    pingGps.lat_e7 = (int32_t)lround(atof(phoneGpsLatBuf) * 1e7);
                    pingGps.lng_e7 = (int32_t)lround(atof(phoneGpsLngBuf) * 1e7);
                    pingGps.batt_pct = heltec_battery_percent(readVbat());
                    haveGps = true;
                }
            }
            if (haveGps) {
                // Never call duck.sendData() from inside recvDataCallback — it races
                // with the radio's TX_DONE ISR and aborts the transmission (same
                // reason handleGps() uses gpsTxPending).
                // If the phone responded to GPSREQ, handleGps() already set
                // gpsTxPayload + gpsTxPending with the full telemetry payload.
                // Only override here for the cached-GPS path.
                if (!gpsTxPending) {
                    gpsTxPayload = duckpayload::encodeGps(pingGps);
                    gpsTxPending = true;
                }
                Serial.printf("[PING] GPS TX deferred: lat_e7=%d lng_e7=%d\n", pingGps.lat_e7, pingGps.lng_e7);
            }
            break;
        }

        case 22:  // Text message / operator command
            if (duckpayload::isProtobuf(packet.data.data(), packet.data.size())) {
              duckcdp_OpText opText = duckcdp_OpText_init_zero;
              if (!duckpayload::decodeOpText(packet.data.data(), packet.data.size(), opText)) {
                Serial.println("[MSG] ERROR: failed to decode OpText payload.");
                break;
              }
              message = String(opText.text);
            }
            Serial.println("📨 Message: " + message);
            if (message.indexOf("SOS DITERIMA") >= 0) {
                // Debounce: ignore duplicate SOS_ACK packets arriving via multiple
                // relay paths within 5 s of the first one.  Relay echoes arrive
                // within ~1-2 s; OpenDMS retransmissions are spaced 10 s apart,
                // so a 5 s window catches mesh duplicates without swallowing retries.
                static unsigned long lastSosAckMs = 0;
                if (millis() - lastSosAckMs < 5000UL) {
                    Serial.println("[MAMA] SOS_ACK duplicate suppressed (debounce)");
                    break;
                }
                lastSosAckMs = millis();
                // SOS acknowledgment from operator — defer display to top of loop()
                // so it is never overwritten by the in-progress sendEmergency() sequence.
                sosAckDisplayPending = true;
                broadcast("CDK:SOS_ACK,TEXT:SOS DITERIMA");
                Serial.println("[MAMA] SOS acknowledged by operator");
                break;
            }
            display.displayOn();
            displayMessage(message);
            emergencyDisplayPending = true;   // operator message — stay until button pressed
            displayEnabled = true;
            {
              duckcdp_OpText ack = duckcdp_OpText_init_zero;
              String ackText = "MSG_READ:TEXT:" + message;
              std::snprintf(ack.text, sizeof(ack.text), "%s", ackText.c_str());
              std::vector<uint8_t> encoded = duckpayload::encodeOpText(ack);
              duck.sendData(22, encoded.data(), (int)encoded.size());
            }
            // blink to show message arrive
            blinkLed(1);
            // send message to phone via both USB serial and Bluetooth Low energy
            broadcast(String("CDK:MSG,TEXT:") + message);
            // send message to phone via USB serial ONLY
            //Serial.println("CDK:MSG,TEXT:" + text);
            break;

        case 23:  // Alert
            if (duckpayload::isProtobuf(packet.data.data(), packet.data.size())) {
              duckcdp_OpText opText = duckcdp_OpText_init_zero;
              if (duckpayload::decodeOpText(packet.data.data(), packet.data.size(), opText)) {
                message = String(opText.text);
              }
            }
            Serial.println("⚠️  ALERT: " + message);
            flashLED();
            broadcast(String("CDK:MSG,TEXT:") + message);
            {
              duckcdp_OpText ack = duckcdp_OpText_init_zero;
              std::snprintf(ack.text, sizeof(ack.text), "%s", "ALERT_ACK");
              std::vector<uint8_t> encoded = duckpayload::encodeOpText(ack);
              duck.sendData(23, encoded.data(), (int)encoded.size());
            }
            break;

        case 24:  // Emergency broadcast from operator
            if (duckpayload::isProtobuf(packet.data.data(), packet.data.size())) {
              duckcdp_OpText opText = duckcdp_OpText_init_zero;
              if (duckpayload::decodeOpText(packet.data.data(), packet.data.size(), opText)) {
                message = String(opText.text);
              }
            }
            Serial.println("📢 Emergency Broadcast: " + message);
            displayAnnouncement(message);
            //flashLED();
            blinkLed(1);
            // Forward to connected phone via USB serial and BLE
            broadcast(String("CDK:BCAST,TEXT:") + message);
            break;
        case 25:
            if (duckpayload::isProtobuf(packet.data.data(), packet.data.size())) {
              duckcdp_OpText opText = duckcdp_OpText_init_zero;
              if (duckpayload::decodeOpText(packet.data.data(), packet.data.size(), opText)) {
                message = String(opText.text);
              }
            }
            Serial.println("Personal message: " + message);
            broadcast(String("CDK:PMSG,TEXT:") + message);
            break;

        case 234: {  // GPS location request (topics::gps = 0xEA)
            Serial.println("[GPS] Location request received");
#ifdef ARDUINO_heltec_wifi_lora_32_V4
            if (tinyGps.location.isValid()) {
              // Hardware GPS is the priority source — send regardless of fix age.
              char gpsBuf[128];
              float altM  = tinyGps.altitude.isValid() ? tinyGps.altitude.meters()  : 0.0f;
              float spdKh = tinyGps.speed.isValid()    ? tinyGps.speed.kmph()        : 0.0f;
              float hdgDeg = tinyGps.course.isValid()  ? tinyGps.course.deg()        : 0.0f;
              std::snprintf(gpsBuf, sizeof(gpsBuf),
                            "GPS,LAT:%.6f,LNG:%.6f,ALT:%.1f,SPD:%.1f,HDG:%.1f,SATS:%u,BATT:%d",
                            tinyGps.location.lat(), tinyGps.location.lng(),
                            altM, spdKh, hdgDeg,
                            tinyGps.satellites.value(),
                            heltec_battery_percent(readVbat()));
              // Show transmission status on OLED before sending.
              display.displayOn();
              display.clear();
              display.setFont(ArialMT_Plain_10);
              display.setTextAlignment(TEXT_ALIGN_LEFT);
              display.drawString(0, 0, "Batt: " + String(heltec_battery_percent(readVbat())) + "%");
              display.setTextAlignment(TEXT_ALIGN_RIGHT);
              display.drawString(128, 0, buffer);
              display.setTextAlignment(TEXT_ALIGN_LEFT);
              display.drawString(0, 14, TXT_SENDING_GPS_DATA);
              display.drawString(0, 28, "LAT:" + String(tinyGps.location.lat(), 5));
              display.drawString(0, 40, "LNG:" + String(tinyGps.location.lng(), 5));
              display.drawString(0, 52, "ALT:" + String(altM, 1) + "m  SPD:" + String(spdKh, 1) + "km/h");
              display.display();
              duck.sendData(topics::gps, std::string(gpsBuf));
              Serial.printf("[GPS] Hardware GPS sent (age: %lums): %s\n",
                            tinyGps.location.age(), gpsBuf);
              // Non-blocking display clear — delay() here would block duck.run() for
              // 3 s, causing the SX1262 FIFO to fill up and corrupt queued packets.
              gpsDisplayClearMs = millis() + 3000;
            } else {
              // No hardware fix — request from phone if connected, else report to mesh.
              bool phoneConnected = isPhoneConnected();
              Serial.printf("[GPS] No hardware fix (module %s) — phone %s\n",
                            gpsModuleDetected ? "detected, awaiting fix" : "not detected",
                            phoneConnected ? "connected, requesting" : "not connected");
              display.displayOn();
              display.clear();
              display.setFont(ArialMT_Plain_10);
              display.setTextAlignment(TEXT_ALIGN_LEFT);
              display.drawString(0, 0, "Batt: " + String(heltec_battery_percent(readVbat())) + "%");
              display.setTextAlignment(TEXT_ALIGN_RIGHT);
              display.drawString(128, 0, buffer);
              display.setTextAlignment(TEXT_ALIGN_CENTER);
              if (phoneConnected) {
                display.drawString(64, 28, TXT_REQ_GPS_DATA_FROM_PHONE_2L);
                display.display();
                // Defer CDK:GPSREQ — sending it immediately after CDK:SEEN causes Android's
                // BLE stack to silently drop one of the two rapid-succession notifications.
                // The main loop sends it 400 ms later, after CDK:SEEN has been processed.
                if (gpsReqDeferredSendMs == 0) {
                  gpsReqDeferredSendMs = millis() + 400;
                }
              } else {
                display.drawString(64, 28, TXT_NO_PHONE_NO_GPS_2L);
                display.display();
                char noGpsBuf[64];
                std::snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_PHONE,BATT:%d",
                              heltec_battery_percent(readVbat()));
                duck.sendData(topics::gps, std::string(noGpsBuf));
                Serial.println("[GPS] No phone connected — sent " + String(noGpsBuf));
              }
              // Non-blocking display clear — delay() here blocks duck.run() for 2 s,
              // preventing the radio from reading queued packets and corrupting the
              // SX1262 FIFO when a second packet arrives before the first is read.
              gpsDisplayClearMs = millis() + 2000;
            }
#else
            // No GPS hardware on this board — request from phone if connected, else report to mesh.
            {
              bool phoneConnected = isPhoneConnected();
              Serial.printf("[GPS] No GPS hardware — phone %s, cache %s\n",
                            phoneConnected ? "connected" : "not connected",
                            phoneGpsLatBuf[0] != '\0' ? "hot" : "empty");
              display.displayOn();
              display.clear();
              display.setFont(ArialMT_Plain_10);
              display.setTextAlignment(TEXT_ALIGN_LEFT);
              display.drawString(0, 0, "Batt: " + String(heltec_battery_percent(readVbat())) + "%");
              display.setTextAlignment(TEXT_ALIGN_RIGHT);
              display.drawString(128, 0, buffer);
              display.setTextAlignment(TEXT_ALIGN_CENTER);
              if (phoneConnected) {
                if (phoneGpsLatBuf[0] != '\0') {
                  // Cache hot — will respond immediately from cache in deferred dispatch
                  display.setTextAlignment(TEXT_ALIGN_LEFT);
                  display.drawString(0, 14, TXT_SENDING_GPS_DATA);
                  display.drawString(0, 28, "LAT:" + String(phoneGpsLatBuf));
                  display.drawString(0, 42, "LNG:" + String(phoneGpsLngBuf));
                } else {
                  display.drawString(64, 28, TXT_REQ_GPS_DATA_FROM_PHONE_2L);
                }
                display.display();
                // Defer GPS response — sending immediately after CDK:SEEN causes Android's
                // BLE stack to silently drop one of the two rapid-succession notifications.
                // The main loop sends GPSREQ (or uses cache) 400 ms later.
                if (gpsReqDeferredSendMs == 0) {
                  gpsReqDeferredSendMs = millis() + 400;
                }
              } else {
                display.drawString(64, 28, TXT_NO_PHONE_NO_GPS_2L);
                display.display();
                char noGpsBuf[64];
                std::snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_PHONE,BATT:%d",
                              heltec_battery_percent(readVbat()));
                duck.sendData(topics::gps, std::string(noGpsBuf));
                Serial.println("[GPS] No phone connected — sent " + String(noGpsBuf));
              }
              // Non-blocking display clear — delay() here blocks duck.run() for 2 s,
              // preventing the radio from reading queued packets and corrupting the
              // SX1262 FIFO when a second packet arrives before the first is read.
              gpsDisplayClearMs = millis() + 2000;
            }
#endif
            break;
        }

        case TOPIC_BEACON: {  // 27 — combined discovery + GPS beacon
            // GPS coordinates were already extracted into duckGpsCache in
            // section 1 above, and CDK:SEEN (with GPS) was already emitted
            // in section 2 above — nothing extra needed for the app side.
            //
            // Reply with our own GPS so the sender can also populate its
            // duckGpsCache and emit a GPS-rich CDK:SEEN on our behalf.
            // Defer the TX to main loop (can't call duck.sendData inside
            // recvDataCallback — TX abort race with the LoRa ISR).
            //
            // Skip relayed copies of our own BEACON — the CDP default relay
            // path re-broadcasts PAPADUCK_DUID packets back at us, so we
            // would otherwise ACK our own packet and create a flag deadlock.
            if (memcmp(packet.sduid.data(), duck.getDuckId().data(), 8) == 0) {
                Serial.println("[BEACON] Ignoring relay of own BEACON");
                break;
            }
            Serial.printf("[BEACON] Received from %.8s — preparing ACK\n",
                          (char*)packet.sduid.data());
            if (!beaconAckPending && !gpsTxPending) {
                char ownGps[80] = {};
#ifdef ARDUINO_heltec_wifi_lora_32_V4
                if (tinyGps.location.isValid() && tinyGps.location.age() < 30000) {
                    snprintf(ownGps, sizeof(ownGps), "GPS,LAT:%.6f,LNG:%.6f",
                             tinyGps.location.lat(), tinyGps.location.lng());
                } else
#endif
                if (phoneGpsLatBuf[0] != '\0') {
                    snprintf(ownGps, sizeof(ownGps), "GPS,LAT:%s,LNG:%s",
                             phoneGpsLatBuf, phoneGpsLngBuf);
                } else {
                    strncpy(ownGps, "GPS,FIX:0", sizeof(ownGps) - 1);
                    Serial.println("[BEACON] No GPS for ACK — requesting from phone (deferred)");
                    if (isPhoneConnected() && gpsReqDeferredSendMs == 0) {
                        gpsReqDeferredSendMs = millis() + 300;
                    }
                }
                strncpy(beaconAckPayload, ownGps, sizeof(beaconAckPayload) - 1);
                beaconAckPending = true;
                Serial.printf("[BEACON] ACK queued: %s\n", ownGps);
            } else {
                Serial.printf("[BEACON] ACK skipped: beaconAckPending=%d gpsTxPending=%d\n",
                              (int)beaconAckPending, (int)gpsTxPending);
            }
            break;
        }

        case TOPIC_BEACON_ACK: {  // 28 — reply to our beacon
            // GPS already extracted in section 1 → duckGpsCache updated.
            // CDK:SEEN (with GPS) already emitted in section 2.
            // Nothing further needed — the app side is already updated.
            if (memcmp(packet.sduid.data(), duck.getDuckId().data(), 8) == 0) {
                Serial.println("[BEACON] Ignoring relay of own BEACON_ACK");
                break;
            }
            Serial.printf("[BEACON] ACK received from %.8s — CDK:SEEN emitted\n",
                          (char*)packet.sduid.data());
            break;
        }

        case 26:  // MamaDuck-to-MamaDuck (MTALK)
            if (duckpayload::isProtobuf(packet.data.data(), packet.data.size())) {
              // ── Protobuf-encoded MTalk (see duck_payloads.proto) ─────────
              duckcdp_MTalk mtalk = duckcdp_MTalk_init_zero;
              if (!duckpayload::decodeMTalk(packet.data.data(), packet.data.size(), mtalk)) {
                Serial.println("[MTALK] ERROR: failed to decode MTalk payload.");
                break;
              }
              String senderId = String((char*)packet.sduid.data(), 8);
              String mid = String(mtalk.mid);
              if (mtalk.kind == duckcdp_MTalkKind_MTALK_ACK) {
                // Delivery receipt coming back to the original sender.
                Serial.println("[MTALK] Received ACK mid=" + mid);
                broadcast("CDK:MACK,ID:" + mid + ",FROM:" + senderId);
              } else {
                // Incoming message.
                String text = String(mtalk.text);
                Serial.println("[MTALK] Received: " + text);
                String frameOut = "CDK:MTALK,TEXT:" + text + ",FROM:" + senderId;
                if (mid.length() > 0) frameOut += ",MID:" + mid;
                broadcast(frameOut);
                // Send targeted delivery receipt back to the original sender.
                if (mid.length() > 0) {
                  std::array<uint8_t, 8> senderDuid;
                  for (int i = 0; i < 8; i++) senderDuid[i] = packet.sduid[i];
                  duckcdp_MTalk ack = duckcdp_MTalk_init_zero;
                  ack.kind = duckcdp_MTalkKind_MTALK_ACK;
                  std::snprintf(ack.mid, sizeof(ack.mid), "%s", mid.c_str());
                  std::vector<uint8_t> encoded = duckpayload::encodeMTalk(ack);
                  if (!encoded.empty()) {
                    duck.sendData(26, encoded.data(), (int)encoded.size(), senderDuid);
                    Serial.println("[MTALK] MACK sent back to " + senderId);
                  }
                }
              }
              break;
            }
            // ── Legacy plain-text MTALK (pre-protobuf firmware) ──────────────
            Serial.println("[MTALK] Received (legacy): " + message);
            if (message.startsWith("[MACK:")) {
              // ── Delivery receipt coming back to the original sender ───────
              // Forward to app: CDK:MACK,ID:<mid>,FROM:<senderDuckId>
              int end = message.indexOf(']', 6);
              String ackId = (end > 6) ? message.substring(6, end) : "";
              String senderId = String((char*)packet.sduid.data(), 8);
              broadcast("CDK:MACK,ID:" + ackId + ",FROM:" + senderId);
            } else {
              // ── Incoming message ─────────────────────────────────────────
              // Strip optional ",MID:<4chars>" suffix embedded by sender firmware.
              String text = message;
              String mid  = "";
              int midIdx = message.lastIndexOf(",MID:");
              if (midIdx >= 0 && (int)(message.length() - midIdx) == 9) {
                mid  = message.substring(midIdx + 5);
                text = message.substring(0, midIdx);
              }
              // Extract sender duck ID from the LoRa packet source field.
              String senderId = String((char*)packet.sduid.data(), 8);
              // Deliver to the connected app with sender info and MID (if any).
              String frameOut = "CDK:MTALK,TEXT:" + text + ",FROM:" + senderId;
              if (mid.length() > 0) frameOut += ",MID:" + mid;
              broadcast(frameOut);
              // Send targeted delivery receipt back to the original sender.
              if (mid.length() > 0) {
                std::array<uint8_t, 8> senderDuid;
                for (int i = 0; i < 8; i++) senderDuid[i] = packet.sduid[i];
                String mack = "[MACK:" + mid + "]";
                duck.sendData(26, std::string(mack.c_str()), senderDuid);
                Serial.println("[MTALK] MACK sent back to " + senderId);
              }
            }
            break;
    }
 }

 void displayMessage(String msg) {
    msg.toUpperCase();
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    //display.println("Message:");
    display.drawStringMaxWidth(0, 10, 128, msg);
    //display.println(msg);
    display.display();
 }

 void displayHome() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    // Row 1 (y=0): BATT left, ID right
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "BATT:" + String(heltec_battery_percent(readVbat())) + "%");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 0, buffer);
    // Row 2 (y=12): signal quality or TX status
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    String sigStr;
    if (lastSignalPct >= 0) {
      // Received-packet signal quality (most accurate)
      if      (lastSignalPct <= 25) sigStr = TXT_SIG_WEAK   + String(lastSignalPct) + "%)";
      else if (lastSignalPct <= 50) sigStr = TXT_SIG_OK   + String(lastSignalPct) + "%)";
      else if (lastSignalPct <= 75) sigStr = TXT_SIG_STRONG    + String(lastSignalPct) + "%)";
      else                          sigStr = TXT_SIG_VSTRONG + String(lastSignalPct) + "%)";
    } else if (lastTxResult == 0) {
      sigStr = TXT_SEND_OK;
    } else if (lastTxResult > 0) {
      sigStr = TXT_SEND_FAIL;
    } else {
      sigStr = TXT_SIG_NONE;
    }
    display.drawString(64, 12, sigStr);

    display.drawString(64, 26, TXT_HOME_HINT_3L);
    display.display();
 }

 void displayID() {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 0, buffer);
    display.display();
 }

/** Read battery voltage with VBAT_CTRL HIGH (library uses LOW which reads 0 on this board). */
static float readVbat() {
  pinMode(VBAT_CTRL, OUTPUT);
  digitalWrite(VBAT_CTRL, HIGH);
  delay(5);
  float vbat = analogRead(VBAT_ADC) / 238.7;
  pinMode(VBAT_CTRL, INPUT);
  return vbat;
  return heltec_vbat();
}

void sendBattery() {
  int pct = heltec_battery_percent(readVbat());
  broadcast("CDK:BATT,LEVEL:" + String(pct));
}

void displayBatt() {
  int pct = heltec_battery_percent(readVbat());
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Batt: " + String(pct) + "%");
  display.display();
}

 void flashLED() {
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
 }

 // Fast, non-blocking-ish LED pulse used for per-tick hold feedback.
 // blinkLed() is too slow (~400ms per blink, serviced by duck.run() in a wait
 // loop) to use for escalating hold-progress ticks without noticeably
 // delaying the next pressedFor() threshold check.
 static void quickBlink() {
   heltec_led(80);
   delay(30);
   heltec_led(0);
 }

 // Live progress bar shown while the button is held toward the 2s SOS
 // threshold. Uses SSD1306Wire's native drawProgressBar() instead of
 // WioTracker's hand-rolled u8g2 drawFrame/drawBox bar.
 void showHoldProgress(uint32_t heldMs) {
   uint8_t pct = (uint8_t)constrain((heldMs * 100UL) / SOS_HOLD_MS, 0UL, 100UL);
   display.clear();
   display.setTextAlignment(TEXT_ALIGN_CENTER);
   display.setFont(ArialMT_Plain_10);
   display.drawString(64, 8, TXT_HOLD_FOR_SOS);
   display.drawProgressBar(4, 28, 120, 12, pct);
   display.display();
 }

 bool sendEmergency(String lat, String lng, String alt, String spd, String hdg, bool gpsFromPhone) {
   bool failure;
   bool hasGps = (lat.length() > 0 && lng.length() > 0);
   int battPct = heltec_battery_percent(readVbat());

   // Protobuf-encode the alert (see duck_payloads.proto: SosAlert) —
   // DeviceID is carried in the MQTT envelope by the gateway so we don't
   // need to repeat it in the payload bytes.
   duckcdp_SosAlert alertMsg = duckcdp_SosAlert_init_zero;
   alertMsg.origin = duckcdp_SosOrigin_SOS_ORIGIN_DEVICE;
   alertMsg.has_gps = hasGps;
   if (hasGps) {
     alertMsg.gps_source = gpsFromPhone ? duckcdp_GpsSource_GPS_SOURCE_PHONE
                                         : duckcdp_GpsSource_GPS_SOURCE_DEVICE;
     alertMsg.lat_e7 = (int32_t)lround(atof(lat.c_str()) * 1e7);
     alertMsg.lng_e7 = (int32_t)lround(atof(lng.c_str()) * 1e7);
     if (alt.length() > 0) alertMsg.alt_m = (int32_t)lround(atof(alt.c_str()));
     if (spd.length() > 0) alertMsg.spd_dkmh = (uint32_t)lround(atof(spd.c_str()) * 10);
     if (hdg.length() > 0) alertMsg.hdg_deg = (uint32_t)lround(atof(hdg.c_str()));
   } else {
     alertMsg.gps_source = duckcdp_GpsSource_GPS_SOURCE_NONE;
   }
   alertMsg.batt_pct = battPct;
   std::vector<uint8_t> encoded = duckpayload::encodeSos(alertMsg);
   Serial.printf("[MAMA] sendEmergency data: %u bytes (hasGps=%d)\n", (unsigned)encoded.size(), hasGps);

   // Send alert upward to PapaDuck — PapaDuck decides whether to re-broadcast.
   failure = duck.sendData(topics::alert, encoded.data(), encoded.size());
   lastTxResult = failure;   // 0 = success; track for signal-line display
   lastTxMs     = millis();
   if (!failure) {
     counter++;
     display.displayOn();
     blinkLed(3);
     // Notify connected phone (USB or BLE) that hardware button SOS was sent
     String sosFrame = "CDK:SOS,SRC:DEVICE"
                       ",LAT:" + (hasGps ? lat : "none") +
                       ",LNG:" + (hasGps ? lng : "none");
     if (hasGps && alt.length() > 0) sosFrame += ",ALT:" + alt;
     if (hasGps && spd.length() > 0) sosFrame += ",SPD:" + spd;
     if (hasGps && hdg.length() > 0) sosFrame += ",HDG:" + hdg;
     if (hasGps && gpsFromPhone)      sosFrame += ",GPS:PHONE";
     sosFrame += ",BATT:" + String(battPct);
     broadcast(sosFrame);
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, hasGps ? TXT_SOS_SENT_GPS_3L : TXT_SOS_SENT_NOGPS_3L);
     if (!hasGps) blinkLed(2);  // extra blinks — distinct warning that the SOS went out without a location

     // no displayID. because this has no clear();
     display.setTextAlignment(TEXT_ALIGN_RIGHT);
     display.setFont(ArialMT_Plain_10);
     display.drawString(128, 0, buffer);
     display.display();
     blinkLed(2);

     // Keep calling duck.run() for 2 s so the TX_DONE interrupt is serviced
     // promptly and the radio re-enters RX mode before the SOS ack arrives.
     // A plain heltec_delay() blocks duck.run(), which leaves the radio stuck
     // in TX state and causes MamaDuck to miss the incoming ack packet.
     /*displayBatt(); */
     {
       unsigned long endMs = millis() + 2000UL;
       while (millis() < endMs) {
         duck.run();
         heltec_loop();
         ::delay(10);
       }
     }

     display.clear();
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, TXT_SOS_SENT_HINT_3L);
     display.display();
     //heltec_delay(5000);
     displayHome();
   } else {
     Serial.println("[MAMA] sendEmergency failed.");
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, TXT_SOS_ERR_2L);
     display.display();
   }
   return true;
 }

// ── Phone connection detection ─────────────────────────────────────
// Returns true if a phone is currently connected via BLE or USB serial.
// On ESP32-S3 native USB CDC (V3/V4), (bool)Serial is true when the host has
// the CDC ACM port open — no need to wait for the phone to send data first.
// On hardware UART boards the fallback is the last-RX timestamp.
static bool isPhoneConnected() {
  if (bleConnected) return true;    // NimBLE callback keeps this accurate
  return usbPhoneSeen;              // set on CDK frame RX; cleared after USB_IDLE_TIMEOUT_MS of silence
}

// ── Broadcast on all active channels ────────────────────────────────
void broadcast(const String& frame) {
  String payload = frame.endsWith("\n") ? frame : frame + "\n";
  // USB Serial
  Serial.print(payload);
  lastUsbTxMs = millis();  // mark USB as transmitting
  // BLE — only if a phone is connected
  if (bleConnected && pTxChar) {
    pTxChar->setValue(payload.c_str());
    pTxChar->notify();
  }
}

void displayAnnouncement(const String& msg) {
    String upper = msg;
    upper.toUpperCase();
    display.displayOn();
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 12, "[EMERGENCY MESSAGE]");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 26, 128, upper);
    display.display();
    emergencyDisplayPending = true;  // hold until program button pressed
    displayEnabled = true;
}

// ── Frame dispatcher ────────────────────────────────────────────────
void handleFrame(const String& line) {
  if (!line.startsWith("CDK:")) return;  // ignore noise

  Serial.printf("[FRAME] Received: %s (via %s)\n",
                line.c_str(), bleConnected ? "BLE" : "USB");
  // Rate-limit ID response to once per 10 s so rapid incoming frames
  // don't flood Android's BLE notification queue and cause a disconnect.
  {
    static unsigned long lastIdBcastMs = 0;
    if (millis() - lastIdBcastMs >= 10000UL) {
      broadcast(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
      lastIdBcastMs = millis();
    }
  }

  String body = line.substring(4);  // strip "CDK:"
  int comma = body.indexOf(',');
  String type = (comma == -1) ? body : body.substring(0, comma);

  // C++ can't switch on strings; map type to enum first
  enum FrameType { FT_UNKNOWN, FT_SOS, FT_MSG, FT_PING, FT_MTALK, FT_GPS, FT_BYE, FT_SCAN };
  FrameType ft = FT_UNKNOWN;
  if      (type == "SOS")   ft = FT_SOS;
  else if (type == "MSG")   ft = FT_MSG;
  else if (type == "PING")  ft = FT_PING;
  else if (type == "MTALK") ft = FT_MTALK;
  else if (type == "GPS")   ft = FT_GPS;
  else if (type == "BYE")   ft = FT_BYE;
  else if (type == "SCAN")  ft = FT_SCAN;

  switch (ft) {
    case FT_SOS:   handleSOS(body);       break;
    case FT_MSG:   handleMsg(body);       break;
    case FT_PING:  /* ID already broadcast above */ break;
    case FT_MTALK: handleMamaTalk(body);  break;
    case FT_GPS:   handleGps(body);       break;
    case FT_SCAN: {
      // Send a BEACON (topic 27) with our GPS embedded in the payload.
      // Nearby ducks running this firmware reply with BEACON_ACK (topic 28)
      // which also carries their GPS — so both sides get a GPS-rich CDK:SEEN
      // from the very first packet, with no separate GPS-request chain.
      // Legacy ducks that don't understand topic 27 still respond to the CDP
      // PING emitted by their own scan, so backward compatibility is preserved.
      char gpsPayload[80] = {};
#ifdef ARDUINO_heltec_wifi_lora_32_V4
      if (tinyGps.location.isValid() && tinyGps.location.age() < 30000) {
        snprintf(gpsPayload, sizeof(gpsPayload), "GPS,LAT:%.6f,LNG:%.6f",
                 tinyGps.location.lat(), tinyGps.location.lng());
      } else
#endif
      if (phoneGpsLatBuf[0] != '\0') {
        snprintf(gpsPayload, sizeof(gpsPayload), "GPS,LAT:%s,LNG:%s",
                 phoneGpsLatBuf, phoneGpsLngBuf);
      } else {
        // No GPS yet — send FIX:0 so the receiver still sees us in CDK:SEEN
        // and knows we exist even without coordinates this cycle.
        strncpy(gpsPayload, "GPS,FIX:0", sizeof(gpsPayload) - 1);
        // Request GPS from phone so next beacon or ACK carries real coordinates.
        if (isPhoneConnected() && gpsReqDeferredSendMs == 0 && gpsReqSentMs == 0) {
          gpsReqDeferredSendMs = millis() + 300;
          Serial.println("[SCAN] Phone GPS cache empty — deferring CDK:GPSREQ");
        }
      }
      int beaconResult = duck.sendData(TOPIC_BEACON, std::string(gpsPayload), BROADCAST_DUID);
      Serial.printf("[SCAN] BEACON result=%d payload=%s\n", beaconResult, gpsPayload);
      broadcast(beaconResult == 0 ? "CDK:STATUS,SCAN:ping_sent" : "CDK:STATUS,SCAN:ping_failed");
      broadcast("CDK:SCAN_ACK");
      break;
    }
    case FT_BYE:
      // Phone is about to disconnect — show Malay disconnect splash immediately
      // rather than waiting for the idle timeout.
      if (bleConnected) {
        bleDisconnectDisplayPending = true;
      } else {
        usbPhoneSeen = false;
        usbDisconnectDisplayPending = true;
      }
      break;
    default:       break;
  }
}

void handleSOS(const String& body) {
  String lat = extractField(body, "LAT");
  String lng = extractField(body, "LNG");
  String alt = extractField(body, "ALT");
  String spd = extractField(body, "SPD");
  String hdg = extractField(body, "HDG");
  int battPct = heltec_battery_percent(readVbat());
  Serial.print("[SOS] LAT="); Serial.print(lat);
  Serial.print(" LNG="); Serial.print(lng);
  Serial.print(" ALT="); Serial.print(alt);
  Serial.print(" SPD="); Serial.print(spd);
  Serial.print(" HDG="); Serial.print(hdg);
  Serial.print(" BATT="); Serial.println(battPct);
  // show message sending
  displayID(); 
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, TXT_SENDING_SOS_2L);
  display.display();
  // construct the message — include phone telemetry + device battery
  duckcdp_SosAlert alertMsg = duckcdp_SosAlert_init_zero;
  alertMsg.origin = duckcdp_SosOrigin_SOS_ORIGIN_PHONE;
  bool hasGps = (lat.length() > 0 && lng.length() > 0);
  alertMsg.has_gps = hasGps;
  if (hasGps) {
    alertMsg.gps_source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
    alertMsg.lat_e7 = (int32_t)lround(atof(lat.c_str()) * 1e7);
    alertMsg.lng_e7 = (int32_t)lround(atof(lng.c_str()) * 1e7);
    if (alt.length() > 0) alertMsg.alt_m = (int32_t)lround(atof(alt.c_str()));
    if (spd.length() > 0) alertMsg.spd_dkmh = (uint32_t)lround(atof(spd.c_str()) * 10);
    if (hdg.length() > 0) alertMsg.hdg_deg = (uint32_t)lround(atof(hdg.c_str()));
  } else {
    alertMsg.gps_source = duckcdp_GpsSource_GPS_SOURCE_NONE;
  }
  alertMsg.batt_pct = battPct;
  std::vector<uint8_t> encoded = duckpayload::encodeStatusReportSos(alertMsg);
  int failure = duck.sendData(topics::status, encoded.data(), encoded.size());
  // blink LED to show activity
  blinkLed(3);
  if (!failure) {
    Serial.println("[MAMA] send ok.");
  } else {
    Serial.println("[MAMA] send failed.");
  }
  broadcast("CDK:ACK,ID:SOS");  // use broadcast so USB and BLE both receive it
  // Display status
  display.displayOn();
  displayID();
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, TXT_SOS_SENT_GPS_3L);

  // no displayID. because this has no clear();
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(128, 0, buffer);
  display.display();

  // Hold success screen until program button is pressed.
  emergencyDisplayPending = true;
  displayEnabled = true;
}

void handleMsg(const String& body) {
  String urgency = extractField(body, "URGENCY");
  String lat     = extractField(body, "LAT");
  String lng     = extractField(body, "LNG");
  String text    = extractField(body, "TEXT");
  Serial.print("[MSG] urgency="); Serial.print(urgency);
  Serial.print(" lat="); Serial.print(lat);
  Serial.print(" lng="); Serial.print(lng);
  Serial.print(" text="); Serial.println(text);

  // Protobuf-encode the message (see duck_payloads.proto: StatusMsg,
  // wrapped in a StatusReport on the `status` topic).
  duckcdp_StatusMsg statusMsg = duckcdp_StatusMsg_init_zero;
  statusMsg.src = duckcdp_StatusMsgSrc_STATUS_MSG_SRC_PHONE;
  std::snprintf(statusMsg.urgency, sizeof(statusMsg.urgency), "%s", urgency.c_str());
  bool hasGps = (lat.length() > 0 && lng.length() > 0);
  statusMsg.has_gps = hasGps;
  if (hasGps) {
    statusMsg.lat_e7 = (int32_t)lround(atof(lat.c_str()) * 1e7);
    statusMsg.lng_e7 = (int32_t)lround(atof(lng.c_str()) * 1e7);
  }
  std::snprintf(statusMsg.text, sizeof(statusMsg.text), "%s", text.c_str());

  // show send message status
  // Display status
  display.displayOn();
  // Reset back screen
  display.clear();
  displayID();
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, TXT_MSG_SENT);
  display.display();
  blinkLed(1);
  displayHome();

  std::vector<uint8_t> encoded = duckpayload::encodeStatusReportMsg(statusMsg);
  int failure = duck.sendData(topics::status, encoded.data(), encoded.size());
  if (!failure) {
    Serial.println("[MAMA] send ok.");
    broadcast("CDK:ACK,ID:MSG");
  } else {
    Serial.println("[MAMA] send failed.");
  }
}

bool sendMamaTalk(const String& targetId, const String& msg, const String& mid) {
  if (targetId.length() != 8) {
    Serial.println("[MTALK] ERROR: targetId must be exactly 8 characters.");
    return false;
  }
  std::array<uint8_t, 8> targetDuid = duckutils::stringToArray<uint8_t, 8>(std::string(targetId.c_str()));
  // Protobuf-encode the chat message (see duck_payloads.proto: MTalk). The
  // receiver echoes `mid` back as a targeted delivery receipt (MTALK_ACK on
  // topic 26) when one is present.
  duckcdp_MTalk mtalk = duckcdp_MTalk_init_zero;
  mtalk.kind = duckcdp_MTalkKind_MTALK_MSG;
  std::snprintf(mtalk.mid, sizeof(mtalk.mid), "%s", mid.c_str());
  std::snprintf(mtalk.text, sizeof(mtalk.text), "%s", msg.c_str());
  std::vector<uint8_t> encoded = duckpayload::encodeMTalk(mtalk);
  if (encoded.empty()) {
    Serial.println("[MTALK] ERROR: failed to encode MTalk message (too long?).");
    return false;
  }
  int failure = duck.sendData(26, encoded.data(), (int)encoded.size(), targetDuid);
  if (!failure) {
    Serial.println("[MAMA] MTALK sent to " + targetId + ": " + msg);
    broadcast("CDK:ACK,ID:MTALK,TARGET:" + targetId);
  } else {
    Serial.println("[MAMA] MTALK send failed.");
  }
  return !failure;
}

void handleMamaTalk(const String& body) {
  String target = extractField(body, "TARGET");
  String text   = extractField(body, "TEXT");
  String mid    = extractField(body, "MID");
  Serial.print("[MTALK] target="); Serial.print(target);
  Serial.print(" text="); Serial.print(text);
  Serial.print(" mid="); Serial.println(mid);
  if (target.length() == 0) {
    Serial.println("[MTALK] ERROR: missing TARGET field.");
    return;
  }
  sendMamaTalk(target, text, mid);
}

void handleGps(const String& body) {
  gpsReqSentMs = 0;  // phone responded — cancel the no-response fallback
  Serial.println("[GPS] Received GPS frame from phone: " + body);
  String lat = extractField(body, "LAT");
  String lng = extractField(body, "LNG");
  if (lat.length() == 0 || lat == "none" || lng.length() == 0 || lng == "none") {
    phoneGpsNoFix = true;
    phoneGpsDisplayPending = true;  // render from main loop (I2C not thread-safe)
    // Defer duck.sendData() to after duck.run() to avoid TX abort race:
    // the stale TX_DONE interrupt from the relay would call startReceive()
    // and abort a GPS response startTransmit() that ran before duck.run().
    duckcdp_GpsReading noFix = duckcdp_GpsReading_init_zero;
    noFix.has_fix = false;
    noFix.source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
    noFix.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NO_SIGNAL;
    noFix.batt_pct = heltec_battery_percent(readVbat());
    gpsTxPayload = duckpayload::encodeGps(noFix);
    gpsTxPending = true;
    return;
  }
  duckcdp_GpsReading reading = duckcdp_GpsReading_init_zero;
  reading.has_fix = true;
  reading.source = duckcdp_GpsSource_GPS_SOURCE_PHONE;
  reading.no_fix_reason = duckcdp_GpsNoFixReason_GPS_REASON_NONE;
  reading.lat_e7 = (int32_t)lround(atof(lat.c_str()) * 1e7);
  reading.lng_e7 = (int32_t)lround(atof(lng.c_str()) * 1e7);
  strncpy(phoneGpsLatBuf, lat.c_str(), sizeof(phoneGpsLatBuf) - 1);
  strncpy(phoneGpsLngBuf, lng.c_str(), sizeof(phoneGpsLngBuf) - 1);
  // Cache optional telemetry for use by hardware SOS button fallback
  String alt = extractField(body, "ALT");
  String spd = extractField(body, "SPD");
  String hdg = extractField(body, "HDG");
  phoneGpsAltBuf[0] = '\0';
  phoneGpsSpdBuf[0] = '\0';
  phoneGpsHdgBuf[0] = '\0';
  if (alt.length() > 0) {
    strncpy(phoneGpsAltBuf, alt.c_str(), sizeof(phoneGpsAltBuf) - 1);
    reading.alt_m = (int32_t)lround(atof(alt.c_str()));
  }
  if (spd.length() > 0) {
    strncpy(phoneGpsSpdBuf, spd.c_str(), sizeof(phoneGpsSpdBuf) - 1);
    reading.spd_dkmh = (uint32_t)lround(atof(spd.c_str()) * 10);
  }
  if (hdg.length() > 0) {
    strncpy(phoneGpsHdgBuf, hdg.c_str(), sizeof(phoneGpsHdgBuf) - 1);
    reading.hdg_deg = (uint32_t)lround(atof(hdg.c_str()));
  }
  reading.batt_pct = heltec_battery_percent(readVbat());
  phoneGpsNoFix = false;
  phoneGpsDisplayPending = true;  // render from main loop (I2C not thread-safe)
  // Defer duck.sendData() to after duck.run() — see comment above.
  gpsTxPayload = duckpayload::encodeGps(reading);
  gpsTxPending = true;
  Serial.printf("[GPS] GPS TX deferred: lat=%s lng=%s\n", lat.c_str(), lng.c_str());
}

String extractField(const String& body, const String& key) {
  String search = key + ":";
  int idx = body.indexOf(search);
  if (idx == -1) return "";
  int start = idx + search.length();
  int end = body.indexOf(',', start);
  return (end == -1) ? body.substring(start) : body.substring(start, end);
}

// ── Send a CDK frame to the app ──────────────────────────────────
void sendFrame(const String& frame) {
  if (!bleConnected || !pTxChar) return;
  String payload = frame;
  if (!payload.endsWith("\n")) payload += "\n";
  pTxChar->setValue(payload.c_str());
  pTxChar->notify();
}

void blinkLed(int frequency) {
    for (int n = 0; n <= frequency; n++) {
        heltec_led(100);
        { unsigned long t = millis(); while (millis()-t < 200) { duck.run(); ::delay(10); } }
        heltec_led(0);
        { unsigned long t = millis(); while (millis()-t < 200) { duck.run(); ::delay(10); } }
    }
}
