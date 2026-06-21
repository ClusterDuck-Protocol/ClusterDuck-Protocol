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
 #include <cstdio>
 #include <arduino-timer.h>
 #include <CDP.h>
 #ifdef SERIAL_PORT_USBVIRTUAL
 #define Serial SERIAL_PORT_USBVIRTUAL
 #endif
 #include <heltec_unofficial.h>
// #include "wifi.h"
 #include "image.h"
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

 #define DUCK_NAME "IBRAHIM1"
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
 static bool isPhoneConnected();

 // --- Global Variables ---
 MamaDuck duck(DUCK_NAME); // Device ID, MUST be 8 bytes and unique from other ducks;
 auto timer = timer_create_default();  // Creating a timer with default settings
 const int INTERVAL_MS = 10000;        // Interval in milliseconds between runSensor call
 int counter = 1;                      // Counter for the sensor data  
 bool setupOK = false;                 // Flag to check if setup is complete
 char buffer[100];
 const char* s = DUCK_NAME;

static NimBLECharacteristic* pTxChar = nullptr;
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
static char          gpsTxPayload[128]       = {};     // payload for deferred GPS TX
static char          phoneGpsLatBuf[20]      = {};
static char          phoneGpsLngBuf[20]     = {};
static char          phoneGpsAltBuf[12]     = {};     // altitude in metres
static char          phoneGpsSpdBuf[12]     = {};     // speed in km/h
static char          phoneGpsHdgBuf[12]     = {};     // heading in degrees
static unsigned long gpsReqSentMs           = 0;      // millis() when CDK:GPSREQ was sent; 0 if none pending
static int           lastSignalPct           = -1;     // last LoRa signal quality (0-100 %), -1 = no packet yet
static unsigned long lastHomeRefreshMs       = 0;      // last time displayHome() was refreshed from the loop
const  unsigned long HOME_REFRESH_MS         = 5000UL; // re-draw home screen every 5 s when display is on
static bool          messagePending          = false;  // true while a received message occupies the screen
static unsigned long messagePendingMs        = 0;      // millis() when message was shown (for auto-dismiss)
const  unsigned long MESSAGE_DISPLAY_MS      = 10000UL; // auto-dismiss received message after 10 s
static int           lastTxResult            = -1;     // -1=never sent, 0=success, >0=fail
static unsigned long lastTxMs               = 0;      // millis() of most recent duck.sendData() call
static volatile bool sosAckDisplayPending   = false;  // set by handleDuckData; rendered at top of loop()

// ── BLE callbacks ─────────────────────────────────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleConnected = true;
    bleConnectDisplayPending = true;  // notify main loop to render splash
    // Request longer connection interval to reduce radio duty cycle.
    // 80*1.25ms=100ms min, 160*1.25ms=200ms max, latency=4, timeout=4000ms.
    pServer->updateConnParams(connInfo.getConnHandle(), 80, 160, 4, 400);
    // Do NOT delay() here — blocking the NimBLE task stalls iOS connection setup.
    // CDK:ID and battery are sent from the main loop on the next iteration
    // once bleConnected == true.
    broadcast("CDK:ID,VALUE:" DUCK_NAME);
    sendBattery();
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

   // BLE init first — must run regardless of duck setup outcome
   // so the device is always discoverable.
   NimBLEDevice::init(DUCK_NAME);
   NimBLEDevice::setPower(ESP_PWR_LVL_P9);
   NimBLEServer* pServer = NimBLEDevice::createServer();
   pServer->setCallbacks(new ServerCallbacks());
   NimBLEService* pSvc = pServer->createService(NUS_SERVICE);
   pTxChar = pSvc->createCharacteristic(NUS_TX_CHAR, NIMBLE_PROPERTY::NOTIFY);
   NimBLECharacteristic* pRxChar = pSvc->createCharacteristic(
     NUS_RX_CHAR, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
   pRxChar->setCallbacks(new RxCallbacks());
   pSvc->start();
   {
     NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
     pAdv->setMinInterval(160);
     pAdv->setMaxInterval(160);
     // ADV_IND: flags + name — iOS reads this on the very first packet.
     NimBLEAdvertisementData advData;
     advData.setFlags(0x06);  // LE General Discoverable | BR/EDR Not Supported
     advData.setName(DUCK_NAME);
     pAdv->setAdvertisementData(advData);
     // SCAN_RSP: NUS service UUID — Android apps that filter by UUID see it here.
     NimBLEAdvertisementData scanData;
     scanData.addServiceUUID(NimBLEUUID(NUS_SERVICE));
     pAdv->setScanResponseData(scanData);
     NimBLEDevice::startAdvertising();
     bleAdvSlowAfterMs = millis() + 30000;
   }
   Serial.println("[BLE] Advertising started");

   if (duck.setupWithDefaults() != DUCK_ERR_NONE) {
     Serial.println("[MAMA] Failed to setup MamaDuck");
     return;
   }
 
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
  std::snprintf(buffer, sizeof(buffer), "ID:%s", s);

  display.init();
  display.flipScreenVertically();

  display.drawXbm(0, 0, taqisystems_small_width, taqisystems_small_height, taqisystems_small_bits);
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
  Serial.println("CDK:ID,VALUE:" DUCK_NAME);
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
      display.drawString(64, 28, "GPS TELEFON\nTIADA ISYARAT");
      display.display();
      delay(2000);
    } else {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 14, gpsLoraOk ? "BERJAYA HANTAR GPS!" : "GAGAL HANTAR GPS!");
      display.drawString(0, 28, "LAT:" + String(phoneGpsLatBuf));
      display.drawString(0, 40, "LNG:" + String(phoneGpsLngBuf));
      display.drawString(0, 52, "SRC:TELEFON");
      display.display();
      delay(3000);
    }
    display.displayOff();
  }

  // BLE connected splash — deferred from onConnect() callback.
  if (bleConnectDisplayPending) {
    bleConnectDisplayPending = false;
    display.displayOn();
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, "BLUETOOTH\nTERSAMBUNG!");
    display.display();
    delay(2000);
    displayHome();
  }

  // BLE disconnected splash — deferred from onDisconnect() callback.
  if (bleDisconnectDisplayPending) {
    bleDisconnectDisplayPending = false;
    display.displayOn();
    displayEnabled = true;
    displayID();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, "BLUETOOTH\nTERPUTUS");
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
    display.drawString(64, 28, "USB BERSIRI\nTERSAMBUNG!");
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
    display.drawString(64, 28, "USB BERSIRI\nTERPUTUS");
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
    display.drawString(64, 22, "SOS DITERIMA!\nBANTUAN SEDANG\nDIHANTAR");
    display.display();
    blinkLed(3);
    messagePending   = true;
    messagePendingMs = millis();
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
    if (!gotGps && isPhoneConnected()) {
      if (phoneGpsLatBuf[0] == '\0') {
        displayID();
        displayBatt();
        display.setFont(ArialMT_Plain_10);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 22, "MEMINTA GPS\nDARIPADA TELEFON...");
        display.display();
        broadcast("CDK:GPSREQ");
        Serial.println("[SOS] Phone GPS cache empty — requesting fresh fix before SOS...");
        unsigned long waitStart = millis();
        while (phoneGpsLatBuf[0] == '\0' && millis() - waitStart < 2500) {
          duck.run();
          delay(50);
        }
        if (phoneGpsLatBuf[0] == '\0') {
          Serial.println("[SOS] Phone GPS timeout — SOS will be sent without coordinates");
        }
      }
      if (phoneGpsLatBuf[0] != '\0') {
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
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "SEDANG HANTAR\nISYARAT KECEMASAN...");
    display.display();

    sendEmergency(gpsLat, gpsLng, gpsAlt, gpsSpd, gpsHdg, /* gpsFromPhone= */ !gotGps && gpsLat.length() > 0);
    //heltec_delay(1000);
  }

  if (button.isSingleClick()) {
    if (messagePending) {
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

  if (button.isTripleClick()) {
    // Send "Roger" confirmation to the rescuer over the LoRa mesh —
    // same topic (topics::status) and payload format as a phone-sent message.
    duck.sendData(topics::status, std::string("MSG,SRC:DEVICE,TEXT:Roger"));
    broadcast("CDK:ACK,ID:ROGER");
    Serial.println("[MAMA] Triple-click: Roger sent");
    display.displayOn();
    displayEnabled = true;
    display.clear();
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 28, "ROGER DIHANTAR!");
    display.display();
    heltec_delay(2000);
    displayHome();
  }

#ifdef ARDUINO_heltec_wifi_lora_32_V4
  if (button.isQuadrupleClick()) {
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
        display.drawString(64, 28, "TARIKH/MASA\nTIADA ISYARAT");
      }
      display.display();
      heltec_delay(2500);

      Serial.printf("[GPS] Quadruple-click: lat=%.5f lng=%.5f sats=%u age=%lums date=%04d/%02d/%02d time=%02d:%02d:%02d\n",
                    tinyGps.location.lat(), tinyGps.location.lng(),
                    tinyGps.satellites.value(), tinyGps.location.age(),
                    y, mo, d, h, mi, sc);
    } else if (gpsModuleDetected) {
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, "GPS: MODUL AKTIF\nMENUNGGU ISYARAT...");
      Serial.println("[GPS] Quadruple-click: module active, no fix yet");
    } else {
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, "GPS: TIADA MODUL");
      Serial.println("[GPS] Quadruple-click: no GPS module detected");
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
      Serial.println("CDK:ID,VALUE:" DUCK_NAME);
      sendBattery();
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

  // BLE advertising runs continuously — not suppressed by USB activity.
  // Transition fast → slow BLE advertising after 30 s
  if (bleAdvertising && !bleConnected && bleAdvSlowAfterMs > 0 && millis() >= bleAdvSlowAfterMs) {
    bleAdvSlowAfterMs = 0;
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    NimBLEDevice::stopAdvertising();
    pAdv->setMinInterval(320);  // 200 ms — slow to ease 2.4 GHz congestion
    pAdv->setMaxInterval(640);  // 400 ms
    NimBLEDevice::startAdvertising();
  }

  // Periodic battery update every 60 s
  if (millis() - lastBattMs >= 60000UL) {
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
    int result = duck.sendData(topics::gps, std::string(gpsTxPayload));
    gpsLoraOk = (result == 0);
    Serial.printf("[GPS] Deferred LoRa TX %s: %s\n", gpsLoraOk ? "OK" : "FAILED", gpsTxPayload);
  }

  // ── GPS request timeout fallback ────────────────────────────────────────
  // If CDK:GPSREQ was sent but the phone never replied within 10 s, report
  // no-fix to the mesh so OpenDMS gets an answer instead of silence.
  if (gpsReqSentMs > 0 && !gpsTxPending && millis() - gpsReqSentMs > 10000UL) {
    gpsReqSentMs = 0;
    char noGpsBuf[72];
    std::snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_RESPONSE,BATT:%d",
                  heltec_battery_percent(readVbat()));
    duck.sendData(topics::gps, std::string(noGpsBuf));
    Serial.println("[GPS] GPSREQ timeout — no response from phone, sent: " + String(noGpsBuf));
  }
 }

 void handleDuckData(CdpPacket packet) {
    bool isForMe = (memcmp(packet.dduid.data(), duck.getDuckId().data(), 8) == 0);
    bool isBroadcast = (packet.dduid[0] == 0xFF);

    if (!isForMe && !isBroadcast) return;

    // Announce the sender duck to the connected phone for peer-discovery.
    // The app uses CDK:SEEN frames to populate the Nearby Ducks list in the
    // Nearby screen so the user can tap a discovered peer to start an MTALK.
    // Only MAMA and LINK ducks are surfaced — PAPA and DETECTOR are not
    // addressable via MTALK so they are excluded.
    if (packet.duckType == DuckType::MAMA || packet.duckType == DuckType::LINK) {
        const char* typeStr = (packet.duckType == DuckType::MAMA) ? "MAMA" : "LINK";
        char seenBuf[32];
        snprintf(seenBuf, sizeof(seenBuf), "CDK:SEEN,ID:%.8s,TYPE:%s",
                 (char*)packet.sduid.data(), typeStr);
        broadcast(seenBuf);
    }

    Serial.println("HANDLING RECEIVING DATA....");

    String message = String((char*)packet.data.data(), packet.data.size());
    char replyMsg[200];

    switch (packet.topic) {
        case 22:  // Text message or operator command
            Serial.println("📨 Message: " + message);
            if (message.indexOf("SOS DITERIMA") >= 0) {
                sosAckDisplayPending = true;
                broadcast("CDK:SOS_ACK,TEXT:SOS DITERIMA");
                Serial.println("[MAMA] SOS acknowledged by operator");
                break;
            }
            display.displayOn();
            displayMessage(message);
            messagePending   = true;           // keep on screen until dismissed or timeout
            messagePendingMs  = millis();         // start auto-dismiss countdown
            std::snprintf(replyMsg, sizeof(replyMsg), "MSG_READ:TEXT:%s", message.c_str());
            duck.sendData(22, replyMsg);
            //duck.sendData(22, "MSG_READ");
            // blink to show message arrive
            blinkLed(1);
            // send message to phone via both USB serial and Bluetooth Low energy
            broadcast(String("CDK:MSG,TEXT:") + message);
            // send message to phone via USB serial ONLY
            //Serial.println("CDK:MSG,TEXT:" + text);
            break;

        case 23:  // Alert
            Serial.println("⚠️  ALERT: " + message);
            flashLED();
            broadcast(String("CDK:MSG,TEXT:") + message);
            duck.sendData(23, "ALERT_ACK");
            break;

        case 24:  // Emergency broadcast from operator
            Serial.println("📢 Emergency Broadcast: " + message);
            displayAnnouncement(message);
            //flashLED();
            blinkLed(1);
            // Forward to connected phone via USB serial and BLE
            broadcast(String("CDK:BCAST,TEXT:") + message);
            break;
        case 25:
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
              display.drawString(0, 14, "MENGHANTAR DATA GPS");
              display.drawString(0, 28, "LAT:" + String(tinyGps.location.lat(), 5));
              display.drawString(0, 40, "LNG:" + String(tinyGps.location.lng(), 5));
              display.drawString(0, 52, "ALT:" + String(altM, 1) + "m  SPD:" + String(spdKh, 1) + "km/h");
              display.display();
              duck.sendData(topics::gps, std::string(gpsBuf));
              Serial.printf("[GPS] Hardware GPS sent (age: %lums): %s\n",
                            tinyGps.location.age(), gpsBuf);
              delay(3000);
              display.displayOff();
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
                display.drawString(64, 28, "MEMINTA DATA GPS\nDARIPADA TELEFON...");
                display.display();
                broadcast("CDK:GPSREQ");
                gpsReqSentMs = millis();  // start timeout — fallback fires in loop() if phone doesn't respond
              } else {
                display.drawString(64, 28, "TIADA TELEFON\nTIADA DATA GPS");
                display.display();
                char noGpsBuf[64];
                std::snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_PHONE,BATT:%d",
                              heltec_battery_percent(readVbat()));
                duck.sendData(topics::gps, std::string(noGpsBuf));
                Serial.println("[GPS] No phone connected — sent " + String(noGpsBuf));
              }
              delay(2000);
              display.displayOff();
            }
#else
            // No GPS hardware on this board — request from phone if connected, else report to mesh.
            {
              bool phoneConnected = isPhoneConnected();
              Serial.printf("[GPS] No GPS hardware — phone %s\n",
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
                display.drawString(64, 28, "MEMINTA DATA GPS\nDARIPADA TELEFON...");
                display.display();
                broadcast("CDK:GPSREQ");
                gpsReqSentMs = millis();  // start timeout — fallback fires in loop() if phone doesn't respond
              } else {
                display.drawString(64, 28, "TIADA TELEFON\nTIADA DATA GPS");
                display.display();
                char noGpsBuf[64];
                std::snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_PHONE,BATT:%d",
                              heltec_battery_percent(readVbat()));
                duck.sendData(topics::gps, std::string(noGpsBuf));
                Serial.println("[GPS] No phone connected — sent " + String(noGpsBuf));
              }
              delay(2000);
              display.displayOff();
            }
#endif
            break;
        }

        case 26:  // MamaDuck-to-MamaDuck (MTALK)
            Serial.println("[MTALK] Received: " + message);
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
      if      (lastSignalPct <= 25) sigStr = "SIG: LEMAH ("   + String(lastSignalPct) + "%)";
      else if (lastSignalPct <= 50) sigStr = "SIG: CUKUP ("   + String(lastSignalPct) + "%)";
      else if (lastSignalPct <= 75) sigStr = "SIG: KUAT ("    + String(lastSignalPct) + "%)";
      else                          sigStr = "SIG: SG.KUAT (" + String(lastSignalPct) + "%)";
    } else if (lastTxResult == 0) {
      sigStr = "BERJAYA HANTAR";
    } else if (lastTxResult > 0) {
      sigStr = "GAGAL HANTAR";
    } else {
      sigStr = "SIG: TIADA ISYARAT";
    }
    display.drawString(64, 12, sigStr);
    // Row 3 (y=26): instruction text
    display.drawString(64, 26, "TEKAN BUTANG ATAS\nSELAMA DUA SAAT UTK\nISYARAT KECEMASAN");
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

 bool sendEmergency(String lat, String lng, String alt, String spd, String hdg, bool gpsFromPhone) {
   bool failure;
   bool hasGps = (lat.length() > 0 && lng.length() > 0);
   int battPct = heltec_battery_percent(readVbat());

   // Human-readable LoRa payload — clearly identifies hardware button origin
   std::string loraMsg = "SOS,SRC:DEVICE,ID:" DUCK_NAME;
   if (hasGps) {
     loraMsg += ",LAT:" + std::string(lat.c_str()) + ",LNG:" + std::string(lng.c_str());
     if (alt.length() > 0) loraMsg += ",ALT:" + std::string(alt.c_str());
     if (spd.length() > 0) loraMsg += ",SPD:" + std::string(spd.c_str());
     if (hdg.length() > 0) loraMsg += ",HDG:" + std::string(hdg.c_str());
     if (gpsFromPhone)      loraMsg += ",GPS:PHONE";
   }
   loraMsg += ",BATT:" + std::to_string(battPct);
   Serial.print("[MAMA] sendEmergency data: ");
   Serial.println(loraMsg.c_str());

   // Send alert upward to PapaDuck — PapaDuck decides whether to re-broadcast.
   failure = duck.sendData(topics::alert, loraMsg);
   lastTxResult = failure;   // 0 = success; track for signal-line display
   lastTxMs     = millis();
   if (!failure) {
     counter++;
     display.displayOn();
     blinkLed(3);
     // Notify connected phone (USB or BLE) that hardware button SOS was sent
     String sosFrame = "CDK:SOS,SRC:DEVICE,ID:" DUCK_NAME
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
     display.drawString(64, 22, hasGps ? "BERJAYA HANTAR\nISYARAT KECEMASAN\nDENGAN GPS!" : "BERJAYA HANTAR\nISYARAT KECEMASAN!");

     // no displayID. because this has no clear();
     display.setTextAlignment(TEXT_ALIGN_RIGHT);
     display.setFont(ArialMT_Plain_10);
     display.drawString(128, 0, buffer);
     display.display();
     blinkLed(2);

     /*displayBatt(); */
     heltec_delay(2000);

     display.clear();
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, "TEKAN BUTANG ATAS\nSELAMA 2 SAAT UTK\nISYARAT KECEMASAN");
     display.display();
     //heltec_delay(5000);
     displayHome();
   } else {
     Serial.println("[MAMA] sendEmergency failed.");
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, "RALAT. TIDAK BOLEH\nHANTAR ISYARAT KECEMASAN");
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
}

// ── Frame dispatcher ────────────────────────────────────────────────
void handleFrame(const String& line) {
  if (!line.startsWith("CDK:")) return;  // ignore noise

  // Re-announce identity so the app recovers device ID after reconnect
  broadcast("CDK:ID,VALUE:" DUCK_NAME);

  String body = line.substring(4);  // strip "CDK:"
  int comma = body.indexOf(',');
  String type = (comma == -1) ? body : body.substring(0, comma);

  // C++ can't switch on strings; map type to enum first
  enum FrameType { FT_UNKNOWN, FT_SOS, FT_MSG, FT_PING, FT_MTALK, FT_GPS, FT_BYE };
  FrameType ft = FT_UNKNOWN;
  if      (type == "SOS")   ft = FT_SOS;
  else if (type == "MSG")   ft = FT_MSG;
  else if (type == "PING")  ft = FT_PING;
  else if (type == "MTALK") ft = FT_MTALK;
  else if (type == "GPS")   ft = FT_GPS;
  else if (type == "BYE")   ft = FT_BYE;

  switch (ft) {
    case FT_SOS:   handleSOS(body);       break;
    case FT_MSG:   handleMsg(body);       break;
    case FT_PING:  /* ID already broadcast above */ break;
    case FT_MTALK: handleMamaTalk(body);  break;
    case FT_GPS:   handleGps(body);       break;
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
  display.drawString(64, 22, "SEDANG HANTAR\nISYARAT KECEMASAN...");
  display.display();
  // construct the message — include phone telemetry + device battery
  String message = "SOS,LAT:" + lat + ",LNG:" + lng;
  if (alt.length() > 0) message += ",ALT:" + alt;
  if (spd.length() > 0) message += ",SPD:" + spd;
  if (hdg.length() > 0) message += ",HDG:" + hdg;
  message += ",BATT:" + String(battPct);  // no \n — sendData handles its own framing
  int failure = duck.sendData(topics::status, std::string(message.c_str()));
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
  display.drawString(64, 22, "BERJAYA HANTAR\nISYARAT KECEMASAN\nDENGAN GPS!");

  // no displayID. because this has no clear();
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(128, 0, buffer);
  display.display();

  heltec_delay(5000);

  // Reset back screen
  display.clear();
  displayID();
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "TEKAN BUTANG ATAS\nSELAMA 2 SAAT UTK\nISYARAT KECEMASAN");
  display.display();
  //heltec_delay(5000);
  displayHome();
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

  String message = "MSG,URGENCY:" + urgency + ",LAT:" + lat + ",LNG:" + lng + ",TEXT:" + text;  // no \n
                                                                                                //
  // show send message status
  // Display status
  display.displayOn();
  // Reset back screen
  display.clear();
  displayID();
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "MESEJ TELAH DIHANTAR!");
  display.display();
  blinkLed(1);
  displayHome();

  int failure = duck.sendData(topics::status, std::string(message.c_str()));
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
  // Embed the MID as a trailing ",MID:<id>" suffix so the receiver can echo it
  // back as a targeted delivery receipt ([MACK:<id>] on topic 26).
  String payload = msg;
  if (mid.length() > 0) payload += ",MID:" + mid;
  int failure = duck.sendData(26, std::string(payload.c_str()), targetDuid);
  if (!failure) {
    Serial.println("[MAMA] MTALK sent to " + targetId + ": " + payload);
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
    std::snprintf(gpsTxPayload, sizeof(gpsTxPayload), "GPS,FIX:0,SRC:PHONE,REASON:NO_SIGNAL,BATT:%d",
                 heltec_battery_percent(readVbat()));
    gpsTxPending = true;
    return;
  }
  char gpsBuf[128];
  std::snprintf(gpsBuf, sizeof(gpsBuf), "GPS,SRC:PHONE,LAT:%s,LNG:%s",
                lat.c_str(), lng.c_str());
  strncpy(phoneGpsLatBuf, lat.c_str(), sizeof(phoneGpsLatBuf) - 1);
  strncpy(phoneGpsLngBuf, lng.c_str(), sizeof(phoneGpsLngBuf) - 1);
  // Cache optional telemetry for use by hardware SOS button fallback
  String alt = extractField(body, "ALT");
  String spd = extractField(body, "SPD");
  String hdg = extractField(body, "HDG");
  phoneGpsAltBuf[0] = '\0';
  phoneGpsSpdBuf[0] = '\0';
  phoneGpsHdgBuf[0] = '\0';
  if (alt.length() > 0) { strncpy(phoneGpsAltBuf, alt.c_str(), sizeof(phoneGpsAltBuf) - 1); strncat(gpsBuf, (",ALT:" + alt).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1); }
  if (spd.length() > 0) { strncpy(phoneGpsSpdBuf, spd.c_str(), sizeof(phoneGpsSpdBuf) - 1); strncat(gpsBuf, (",SPD:" + spd).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1); }
  if (hdg.length() > 0) { strncpy(phoneGpsHdgBuf, hdg.c_str(), sizeof(phoneGpsHdgBuf) - 1); strncat(gpsBuf, (",HDG:" + hdg).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1); }
  // Append battery last so telemetry fields stay grouped
  char battSuffix[16];
  std::snprintf(battSuffix, sizeof(battSuffix), ",BATT:%d", heltec_battery_percent(readVbat()));
  strncat(gpsBuf, battSuffix, sizeof(gpsBuf) - strlen(gpsBuf) - 1);
  phoneGpsNoFix = false;
  phoneGpsDisplayPending = true;  // render from main loop (I2C not thread-safe)
  // Defer duck.sendData() to after duck.run() — see comment above.
  strncpy(gpsTxPayload, gpsBuf, sizeof(gpsTxPayload) - 1);
  gpsTxPending = true;
  Serial.printf("[GPS] GPS TX deferred: %s\n", gpsBuf);
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
