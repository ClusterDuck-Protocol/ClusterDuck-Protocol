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

 #define HELTEC_POWER_BUTTON

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

 #define DUCK_NAME "ZAIHAN12"
 // Bluetooth Low energgy definitions
 #define NUS_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
 #define NUS_RX_CHAR "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
 #define NUS_TX_CHAR "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

 
 // --- Function Declarations ---
 bool runSensor(void *);
 bool sendEmergency();
 void handleDuckData(CdpPacket packet);
 void displayMessage(String msg);
 void displayAnnouncement(const String& msg);
 void flashLED();
 void displayID();
 void displayBatt();
 void handleFrame(const String& line);
 void broadcast(const String& frame);
 void handleSOS(const String& body);
 void handleMsg(const String& body);
 void handleMamaTalk(const String& body);
 bool sendMamaTalk(const String& targetId, const String& msg, const String& mid = "");
 String extractField(const String& body, const String& key);
 void sendFrame(const String& frame);
 
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
static bool bleAdvertising = true;       // track advertising state
const unsigned long USB_IDLE_TIMEOUT_MS = 30000UL; // resume BLE after 30 s of USB silence
 // static unsigned long lastBattMs = 0; battery for later

// ── BLE callbacks ─────────────────────────────────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& connInfo) override {
    bleConnected = true;
    // The app will send CDK:PING after subscribing; handleFrame() will reply
    // with CDK:ID. Keep a short delay as a fallback for older app versions.
    delay(500);
    broadcast("CDK:ID,VALUE:" DUCK_NAME);
    //sendBattery();
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo& connInfo, int reason) override {
    bleConnected = false;
    bleInBuf = "";
    // Only restart advertising if USB is not currently active
    bool usbActive = (lastUsbRxMs > 0 && millis() - lastUsbRxMs < USB_IDLE_TIMEOUT_MS);
    if (!usbActive) {
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

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  std::snprintf(buffer, sizeof(buffer), "ID: %s", s);

  display.init();
  display.flipScreenVertically();

  display.drawXbm(34,14, taqisystems_small_width, taqisystems_small_height, taqisystems_small_bits);
  display.display();
  heltec_delay(10000);
  display.clear();

  displayID();
  displayBatt();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "Tekan butang atas\nsekali untuk hantar\nmesej kecemasan.");
  display.display();

  duck.onReceiveDuckData(handleDuckData);
  heltec_delay(5000);
  display.displayOff();

 Serial.begin(115200);
  delay(200);

  // Send ID + battery over USB immediately
  Serial.println("CDK:ID,VALUE:" DUCK_NAME);
  Serial.println("[MAMA] Firmware v2 (with LAT/LNG support)");
  //sendBattery();

  // BLE init
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
  NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
  // Put the NUS UUID in the advertising packet (not scan response) so Android
  // discovers it without needing a scan response exchange.
  NimBLEAdvertisementData advData;
  advData.setName(DUCK_NAME);
  advData.setCompleteServices(NimBLEUUID(NUS_SERVICE));
  pAdv->setAdvertisementData(advData);
  NimBLEDevice::startAdvertising();

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

   //timer.tick();

  heltec_loop();
  // Button
  if (button.isSingleClick()) {
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "Sedang menghantar\nmesej kecemasan...");
    display.display();

    sendEmergency();
    heltec_delay(1000);
  }

  if (button.isDoubleClick()) {
    display.displayOn();
  }

  // USB incoming
  while (Serial.available()) {
    lastUsbRxMs = millis();  // mark USB as active
    char c = Serial.read();
    if (c == '\n') { handleFrame(usbInBuf); usbInBuf = ""; }
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
  // Periodic battery
  /*if (millis() - lastBattMs >= 60000UL) {
    sendBattery();
    lastBattMs = millis();
  }*/
  delay(5);


  duck.run();
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
            displayMessage(message);
            std::snprintf(replyMsg, sizeof(replyMsg), "MSG_READ:TEXT:%s", message);
            duck.sendData(22, replyMsg);
            //duck.sendData(22, "MSG_READ");
            // send message to phone via both USB serial and Bluetooth Low energy 
            broadcast(String("CDK:MSG,TEXT:") + message);
            // send message to phone via USB serial ONLY
            //Serial.println("CDK:MSG,TEXT:" + text);
            break;

        case 23:  // Alert
            Serial.println("⚠️  ALERT: " + message);
            flashLED();
            duck.sendData(23, "ALERT_ACK");
            break;

        case 24:  // Emergency broadcast from operator
            Serial.println("📢 Emergency Broadcast: " + message);
            displayAnnouncement(message);
            flashLED();
            // Forward to connected phone via USB serial and BLE
            broadcast(String("CDK:BCAST,TEXT:") + message);
            break;
        case 25:
            Serial.println("Personal message: " + message);
            broadcast(String("CDK:PMSG:,TEXT:") + message);
            break;

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
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    //display.println("Message:");
    display.drawStringMaxWidth(0, 10, 128, msg);
    //display.println(msg);
    display.display();
 }

 void displayID() {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 0, buffer);
    display.display();
 }

 void displayBatt() {
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Batt: 100%");
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

 bool sendEmergency() {
   bool failure;

   // Human-readable LoRa payload — clearly identifies hardware button origin
   std::string loraMsg = "SOS,SRC:DEVICE,ID:" DUCK_NAME;
   Serial.print("[MAMA] sendEmergency data: ");
   Serial.println(loraMsg.c_str());

   failure = duck.sendData(topics::alert, loraMsg);
   if (!failure) {
     counter++;
     // Notify connected phone (USB or BLE) that hardware button SOS was sent
     broadcast("CDK:SOS,SRC:DEVICE,ID:" DUCK_NAME ",LAT:none,LNG:none");
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, "Berjaya hantar\nmesej kecemasan!");

     // no displayID. because this has no clear();
     display.setTextAlignment(TEXT_ALIGN_RIGHT);
     display.setFont(ArialMT_Plain_10);
     display.drawString(128, 0, buffer);
     display.display();

     displayBatt();
     heltec_delay(3000);

     display.clear();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, "Tekan butang atas\nsekali untuk hantar\nmesej kecemasan.");
     display.display();
     heltec_delay(5000);
     display.displayOff();
   } else {
     Serial.println("[MAMA] sendEmergency failed.");
     displayID();
     displayBatt();
     display.setFont(ArialMT_Plain_10);
     display.setTextAlignment(TEXT_ALIGN_CENTER);
     display.drawString(64, 22, "Ralat. Tidak boleh\nhantar mesej kecemasan.");
     display.display();
   }
   return true;
 }

// ── Broadcast on all active channels ────────────────────────────────
void broadcast(const String& frame) {
  String payload = frame.endsWith("\n") ? frame : frame + "\n";
  // USB Serial
  Serial.print(payload);
  // BLE — only if a phone is connected
  if (bleConnected && pTxChar) {
    pTxChar->setValue(payload.c_str());
    pTxChar->notify();
  }
}

void displayAnnouncement(const String& msg) {
    displayID();
    displayBatt();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 12, "[EMERGENCY MESSAGE]");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 26, 128, msg);
    display.display();
    heltec_delay(8000);
    display.displayOff();
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
  enum FrameType { FT_UNKNOWN, FT_SOS, FT_MSG, FT_PING, FT_MTALK };
  FrameType ft = FT_UNKNOWN;
  if      (type == "SOS")   ft = FT_SOS;
  else if (type == "MSG")   ft = FT_MSG;
  else if (type == "PING")  ft = FT_PING;
  else if (type == "MTALK") ft = FT_MTALK;

  switch (ft) {
    case FT_SOS:   handleSOS(body);       break;
    case FT_MSG:   handleMsg(body);       break;
    case FT_PING:  /* ID already broadcast above */ break;
    case FT_MTALK: handleMamaTalk(body);  break;
    default:       break;
  }
}

void handleSOS(const String& body) {
  String lat = extractField(body, "LAT");
  String lng = extractField(body, "LNG");
  Serial.print("[SOS] LAT="); Serial.print(lat);
  Serial.print(" LNG="); Serial.println(lng);

  String message = "SOS,LAT:" + lat + ",LNG:" + lng;  // no \n — sendData handles its own framing
  int failure = duck.sendData(topics::status, std::string(message.c_str()));
  if (!failure) {
    Serial.println("[MAMA] send ok.");
  } else {
    Serial.println("[MAMA] send failed.");
  }
  broadcast("CDK:ACK,ID:SOS");  // use broadcast so USB and BLE both receive it
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
