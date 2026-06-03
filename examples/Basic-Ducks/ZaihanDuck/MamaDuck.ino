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

 #define DUCK_NAME "MUHAMMAD"
 // Bluetooth Low energgy definitions
 #define NUS_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
 #define NUS_RX_CHAR "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
 #define NUS_TX_CHAR "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

 
 // --- Function Declarations ---
 bool runSensor(void *);
 bool sendEmergency(String lat = "", String lng = "");
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
static volatile bool phoneGpsDisplayPending = false;
static volatile bool phoneGpsNoFix          = false;
static volatile bool gpsLoraOk              = false;  // result of last duck.sendData() for GPS
static volatile bool gpsTxPending           = false;  // deferred GPS LoRa TX requested by handleGps()
static char          gpsTxPayload[80]       = {};     // payload for deferred GPS TX
static char          phoneGpsLatBuf[20]     = {};
static char          phoneGpsLngBuf[20]     = {};
static unsigned long gpsReqSentMs           = 0;      // millis() when CDK:GPSREQ was sent; 0 if none pending

// ── BLE callbacks ─────────────────────────────────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleConnected = true;
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

  // BLE init
  NimBLEDevice::init(DUCK_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9 dBm — maximum ESP32 TX power for best range/discoverability
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  NimBLEService* pSvc = pServer->createService(NUS_SERVICE);
  pTxChar = pSvc->createCharacteristic(NUS_TX_CHAR, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* pRxChar = pSvc->createCharacteristic(
    NUS_RX_CHAR, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  pRxChar->setCallbacks(new RxCallbacks());
  pSvc->start();
  NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
  // Advertising at 200–400 ms to reduce 2.4 GHz congestion when multiple ducks
  // are deployed nearby. Units: 0.625 ms per BLE spec (320=200 ms, 640=400 ms).
  // Fast advertising for first 30 s after boot — phones discover the device quickly.
  // The loop() transitions to slow (200–400 ms) after bleAdvSlowAfterMs expires.
  pAdv->setMinInterval(160);  // 100 ms — fast initial advertising
  pAdv->setMaxInterval(160);
  // Primary advertising packet: device name.
  // Phones get the name on the very first advertising event without needing
  // to request a scan response — far more reliable on Android where scan
  // response delivery is inconsistent.
  // Budget: Flags (3 B) + name header (2 B) + "ZAIHAN12" (8 B) = 13 B — well
  // within the 31-byte ADV_IND payload limit.
  NimBLEAdvertisementData advData;
  advData.setName(DUCK_NAME);
  pAdv->setAdvertisementData(advData);
  // Scan response: NUS service UUID.
  // Kept here for central devices that want to filter by service UUID.
  NimBLEAdvertisementData scanData;
  scanData.setCompleteServices(NimBLEUUID(NUS_SERVICE));
  pAdv->setScanResponseData(scanData);
  NimBLEDevice::startAdvertising();
  bleAdvSlowAfterMs = millis() + 30000;  // transition to slow advertising after 30 s

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

   //timer.tick();

  heltec_loop();
  // Button
  if (button.pressedFor(2000)) {
    String gpsLat = "";
    String gpsLng = "";
#ifdef ARDUINO_heltec_wifi_lora_32_V4
    if (tinyGps.location.isValid() && tinyGps.location.age() < 5000) {
      gpsLat = String(tinyGps.location.lat(), 6);
      gpsLng = String(tinyGps.location.lng(), 6);
      Serial.printf("[GPS] Fix: lat=%s lng=%s\n", gpsLat.c_str(), gpsLng.c_str());
    } else {
      Serial.println("[GPS] No valid fix — sending SOS without coordinates");
    }
#endif
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "SEDANG HANTAR\nISYARAT KECEMASAN...");
    display.display();

    sendEmergency(gpsLat, gpsLng);
    //heltec_delay(1000);
  }

  if (button.isSingleClick()) {
    displayEnabled = !displayEnabled;
    if (displayEnabled) {
      display.displayOn();
      displayHome();
    } else {
      display.displayOff();
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
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 14, "LAT:" + String(tinyGps.location.lat(), 5));
      display.drawString(0, 26, "LNG:" + String(tinyGps.location.lng(), 5));
      display.drawString(0, 38, "SATS:" + String(tinyGps.satellites.value()));
      display.drawString(0, 50, "AGE:" + String(tinyGps.location.age()) + "ms");
      Serial.printf("[GPS] Triple-click: lat=%.5f, lng=%.5f, sats=%u\n",
                    tinyGps.location.lat(), tinyGps.location.lng(),
                    tinyGps.satellites.value());
    } else if (gpsModuleDetected) {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, "GPS: MODUL AKTIF\nMENUNGGU ISYARAT...");
      Serial.println("[GPS] Triple-click: module active, no fix yet");
    } else {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 28, "GPS: TIADA MODUL");
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
      if (usbInBuf.startsWith("CDK:")) usbPhoneSeen = true;  // phone sent a valid frame
      handleFrame(usbInBuf);
      usbInBuf = "";
    }
    else if (c != '\r') usbInBuf += c;
  }

  // Stop BLE advertising while USB serial is active; resume after idle timeout
  if (lastUsbRxMs > 0 && millis() - lastUsbRxMs < USB_IDLE_TIMEOUT_MS) {
    // USB is active — stop advertising to save power
    if (bleAdvertising && !bleConnected) {
      NimBLEDevice::stopAdvertising();
      bleAdvertising = false;
      Serial.println("[BLE] USB active — advertising paused.");
    }
  } else {
    // USB idle / not connected — resume advertising
    if (!bleAdvertising && !bleConnected) {
      NimBLEDevice::startAdvertising();
      bleAdvertising = true;
      Serial.println("[BLE] USB idle — advertising resumed.");
    }
  }
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

    Serial.println("HANDLING RECEIVING DATA....");

    String message = String((char*)packet.data.data(), packet.data.size());
    char replyMsg[200];

    switch (packet.topic) {
        case 22:  // Text message
            Serial.println("📨 Message: " + message);
            display.displayOn();
            displayMessage(message);
            std::snprintf(replyMsg, sizeof(replyMsg), "MSG_READ:TEXT:%s", message);
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
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 0, buffer);          // ID top-right
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "BATT:" + String(heltec_battery_percent(readVbat())) + "%"); // battery top-left
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "TEKAN BUTANG ATAS\nSELAMA DUA SAAT UTK\n ISYARAT KECEMASAN");
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

 bool sendEmergency(String lat, String lng) {
   bool failure;
   bool hasGps = (lat.length() > 0 && lng.length() > 0);

   // Human-readable LoRa payload — clearly identifies hardware button origin
   std::string loraMsg = "SOS,SRC:DEVICE,ID:" DUCK_NAME;
   if (hasGps) {
     loraMsg += ",LAT:" + std::string(lat.c_str()) + ",LNG:" + std::string(lng.c_str());
   }
   Serial.print("[MAMA] sendEmergency data: ");
   Serial.println(loraMsg.c_str());

   // Send alert upward to PapaDuck — PapaDuck decides whether to re-broadcast.
   failure = duck.sendData(topics::alert, loraMsg);
   if (!failure) {
     counter++;
     display.displayOn();
     blinkLed(3);
     // Notify connected phone (USB or BLE) that hardware button SOS was sent
     String sosFrame = "CDK:SOS,SRC:DEVICE,ID:" DUCK_NAME
                       ",LAT:" + (hasGps ? lat : "none") +
                       ",LNG:" + (hasGps ? lng : "none");
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
  if (bleConnected) return true;
  if (usbPhoneSeen) return true;   // phone has sent a CDK frame via USB
  // (bool)Serial checks DTR which Android apps typically don't assert, so we
  // also treat USB as live if we've recently written frames to it — i.e. the
  // periodic announcement loop is running, meaning the port is at least open.
  bool usbTxRecent = (lastUsbTxMs > 0 && millis() - lastUsbTxMs < GPS_PHONE_TIMEOUT_MS);
  if (usbTxRecent) return true;
  return (lastUsbRxMs > 0 && millis() - lastUsbRxMs < GPS_PHONE_TIMEOUT_MS);
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
  enum FrameType { FT_UNKNOWN, FT_SOS, FT_MSG, FT_PING, FT_MTALK, FT_GPS };
  FrameType ft = FT_UNKNOWN;
  if      (type == "SOS")   ft = FT_SOS;
  else if (type == "MSG")   ft = FT_MSG;
  else if (type == "PING")  ft = FT_PING;
  else if (type == "MTALK") ft = FT_MTALK;
  else if (type == "GPS")   ft = FT_GPS;

  switch (ft) {
    case FT_SOS:   handleSOS(body);       break;
    case FT_MSG:   handleMsg(body);       break;
    case FT_PING:  /* ID already broadcast above */ break;
    case FT_MTALK: handleMamaTalk(body);  break;
    case FT_GPS:   handleGps(body);       break;
    default:       break;
  }
}

void handleSOS(const String& body) {
  String lat = extractField(body, "LAT");
  String lng = extractField(body, "LNG");
  Serial.print("[SOS] LAT="); Serial.print(lat);
  Serial.print(" LNG="); Serial.println(lng);
  // show message sending
  displayID(); 
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "SEDANG HANTAR\nISYARAT KECEMASAN...");
  display.display();
  // construct the message
  String message = "SOS,LAT:" + lat + ",LNG:" + lng;  // no \n — sendData handles its own framing
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
  char gpsBuf[80];
  std::snprintf(gpsBuf, sizeof(gpsBuf), "GPS,SRC:PHONE,LAT:%s,LNG:%s,BATT:%d",
                lat.c_str(), lng.c_str(), heltec_battery_percent(readVbat()));
  strncpy(phoneGpsLatBuf, lat.c_str(), sizeof(phoneGpsLatBuf) - 1);
  strncpy(phoneGpsLngBuf, lng.c_str(), sizeof(phoneGpsLngBuf) - 1);
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
    for (int n = 0; n <= frequency; n++) { heltec_led(100); delay(1000); heltec_led(0); delay(1000); }
}
