/**
 * @file MamaDuck.ino — Seeed Wio Tracker L1 Pro (nRF52840 + SX1262)
 * @brief MamaDuck example using ClusterDuck Protocol on the nRF52840 platform.
 *
 * Hardware: Seeed Wio Tracker L1 Pro
 *   - MCU:     nRF52840 @ 64 MHz
 *   - Radio:   SX1262 (SPI, TCXO 1.8 V, DIO2 RF switch)
 *   - Display: SH1106 128×64 OLED (I2C)
 *   - GPS:     L76KB NMEA (Serial1, 9600 baud)
 *
 * Ported from examples/Basic-Ducks/Seeed/MamaDuck.ino (ESP32 / NimBLE / Heltec).
 * Platform differences:
 *   - U8g2 replaces the Heltec SSD1306 display library.
 *   - Custom battery ADC replaces heltec_battery_percent().
 *   - Manual button debounce replaces the HotButton library.
 *   - TinyGPSPlus on Serial1 replaces UART1 custom GPS setup.
 *
 * @date 2025-07-01
 */

#include <string>
#include <cstdio>
#include <cstring>
#include <map>
#include <CDP.h>
#include <U8g2lib.h>
#include <TinyGPSPlus.h>
#include <bluefruit.h>   // Adafruit Bluefruit52Lib — Nordic UART Service

#include "image.h"
#include "Lang.h"

// ADC_RESOLUTION is not defined in this board's variant.h; 14-bit gives
// full-scale 16383 and must match the analogReadResolution(14) call in setup().
#ifndef ADC_RESOLUTION
#  define ADC_RESOLUTION 14
#endif

// Access the RadioLib radio instance from DuckLoRa.cpp to read RSSI/SNR.
extern CDPCFG_LORA_CLASS lora;

// ── Board sanity check ────────────────────────────────────────────────────────
#ifndef ARDUINO_SEEED_WIO_TRACKER_L1
#error "This sketch is for the Seeed Wio Tracker L1 Pro. \
Define ARDUINO_SEEED_WIO_TRACKER_L1 (or use env:local_wio_tracker_l1)."
#endif

#define DUCK_ID "IBRAHIM1"
// ── Identification ────────────────────────────────────────────────────────────
// Duck ID: MUST be exactly 8 bytes and unique on the mesh.
// To pin a fixed, human-readable ID, `#define DUCK_ID "MYDUCK01"` above this
// line (exactly 8 characters). If DUCK_ID is left undefined, one is
// auto-derived below from this board's factory-unique BLE device address
// (see duckesp::getDuckMacAddress()), so every device gets a distinct,
// reboot-stable ID with no manual configuration required.
// NOTE: the ID-generation function itself lives further down (after enum
// BtnEvent) so it isn't the first function definition in the file --
// Arduino's ctags-based prototype generator inserts all forward-declared
// prototypes right before the first function definition it finds, and
// those prototypes must come after BtnEvent's declaration.
static char DUCK_ID_BUF[9] = {0};
#define DUCK_NAME DUCK_ID_BUF   // kept so the rest of this sketch is unchanged

// ── GPS ───────────────────────────────────────────────────────────────────────
static TinyGPSPlus tinyGps;
static bool gpsModuleDetected = false;
static bool gpsFix            = false;

// ── Display ───────────────────────────────────────────────────────────────────
// SH1106 128×64 software I2C (bit-bang GPIO).
// SW I2C bypasses TWIM entirely, so it works both before AND after
// sd_softdevice_enable() — unlike Wire/HW-I2C which hangs post-sd_enable.
// SCL = D15 = P0.05, SDA = D14 = P0.06 (same physical pins as HW I2C).
U8G2_SH1106_128X64_NONAME_F_SW_I2C display(U8G2_R0, /*clock/SCL*/15, /*data/SDA*/14, U8X8_PIN_NONE);

// U8g2 uses baseline y-coordinates.  These helpers mirror the heltec library's
// top-left convention so ported code can use integer pixel rows (0 = top).
// All measurements are for u8g2_font_6x10_tf:  ascent = 8 px, height = 10 px.
#define DSP_LINE_H  13   // pixel rows between lines (generous for readability)
#define DSP_ASCENT   8   // ascent of u8g2_font_6x10_tf

// Declare here so Arduino's auto-prototype generator sees it before any function
// that returns this type (enum must precede its first use in generated .cpp).
enum BtnEvent { BTN_NONE, BTN_SINGLE, BTN_DOUBLE, BTN_TRIPLE, BTN_QUAD, BTN_HOLD_2S };

// Fills DUCK_ID_BUF with either the user-defined DUCK_ID literal or an
// 8-char ID auto-derived from this board's factory-unique BLE device
// address. Called once, below, before DUCK_ID_BUF's first use.
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

// Draw string starting at top-left pixel (x, y_from_top).
static inline void dspStr(int x, int y, const char* s) {
    display.drawStr(x, y + DSP_ASCENT, s);
}
// Draw centered string.
static inline void dspStrCenter(int y, const char* s) {
    int w = display.getStrWidth(s);
    display.drawStr((128 - w) / 2, y + DSP_ASCENT, s);
}
// Draw right-aligned string.
static inline void dspStrRight(int y, const char* s) {
    int w = display.getStrWidth(s);
    display.drawStr(128 - w, y + DSP_ASCENT, s);
}
// Draw a multi-line string (split on '\n').  y = top of first line.
static void dspMulti(int x, int y, const char* text) {
    char buf[200];
    strncpy(buf, text, sizeof(buf) - 1);
    char* tok = strtok(buf, "\n");
    while (tok) {
        display.drawStr(x, y + DSP_ASCENT, tok);
        y += DSP_LINE_H;
        tok = strtok(nullptr, "\n");
    }
}
// draw() wrapper: clear, set font, draw, send.
static void dspBegin() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
}
static bool gDisplayOk = false;   // true once display.begin() succeeds
static void dspEnd() {
    if (!gDisplayOk) return;   // skip I2C send if display not found
    display.sendBuffer();
}
static inline void dspPowerSave(uint8_t on) {
    if (gDisplayOk) display.setPowerSave(on);
}

// Probe and initialise the SH1106 OLED early in setup().
// Address is hardcoded 0x3D (confirmed by scan; SA0 pin is HIGH on this board).
// No Wire/TWIM used — SW I2C bit-bangs GPIO directly, so TWIM never claims
// pins D14/D15 and display works both before and after sd_softdevice_enable().
static void initDisplay() {
    // Give the display time to power up before first I2C access.
    delay(50);

    // SH1106 address 0x3D (7-bit) → 0x7A (8-bit as U8G2 expects).
    display.setI2CAddress(0x3D << 1);
    if (!display.begin()) {
        // begin() failed — 6 fast blinks.
        for (int i = 0; i < 6; i++) {
            NRF_P1->OUTSET = (1u<<1); delay(80);
            NRF_P1->OUTCLR = (1u<<1); delay(80);
        }
        Serial.println("[DISP] ERROR: display.begin() failed"); Serial.flush();
        return;
    }
    display.setContrast(255);
    display.setPowerSave(0);
    display.setFont(u8g2_font_6x10_tf);
    gDisplayOk = true;
    Serial.println("[DISP] init OK (SW I2C, addr=0x3D)"); Serial.flush();
}
// Show 1-2 centred status lines.  No-op if display not found.
static void dspStatus(const char* line1, const char* line2 = nullptr) {
    if (!gDisplayOk) return;
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    dspStrCenter(line2 ? 20 : 28, line1);
    if (line2) dspStrCenter(36, line2);
    display.sendBuffer();
}

// ── BLE state ────────────────────────────────────────────────────────────────
// Modelled directly on MeshCore's SerialBLEInterface (nrf52/SerialBLEInterface.cpp).
// No PIN/pairing — open NUS (Nordic UART Service).  BLE is initialised last in
// setup(), after all other hardware, mirroring MeshCore's setup() ordering.
static BLEUart  bleuart;
static bool     blePhoneSeen          = false;
static String   bleInBuf              = "";
static volatile bool bleRxPending     = false;
static char     bleRxLine[512]        = {};
static bool     bleAdvertisingStarted = false;
static unsigned long lastBleHealthMs  = 0;
#define BLE_HEALTH_INTERVAL_MS 10000UL



// BLE connection params (from MeshCore): units 1.25 ms / 10 ms.
#define BLE_MIN_CONN_INTERVAL  12   // 15 ms
#define BLE_MAX_CONN_INTERVAL  24   // 30 ms
#define BLE_SLAVE_LATENCY       4
#define BLE_CONN_SUP_TIMEOUT  200   // 2000 ms

static void setupBLE();
static void bleSendLine(const String& line);

// ── USB Serial state ──────────────────────────────────────────────────────────
static String         usbInBuf            = "";
static unsigned long  lastUsbRxMs         = 0;
static bool           usbPhoneSeen        = false;
const  unsigned long  USB_IDLE_TIMEOUT_MS = 30000UL;

// ── Display state ─────────────────────────────────────────────────────────────
static bool           displayEnabled      = true;

// ── Deferred / pending display flags ─────────────────────────────────────────
static volatile bool  phoneGpsDisplayPending     = false;
static volatile bool  phoneGpsNoFix              = false;
static volatile bool  gpsLoraOk                  = false;
static volatile bool  gpsTxPending               = false;
static volatile bool  usbConnectDisplayPending   = false;
static volatile bool  usbDisconnectDisplayPending= false;
static volatile bool  sosAckDisplayPending       = false;

// ── SOS state (non-blocking) ─────────────────────────────────────────────────
// Set when the 2s-hold has fired but we're waiting (asynchronously) on a
// phone GPS reply before actually transmitting. A single click while this is
// true cancels the pending SOS (see BTN_SINGLE handling in loop()).
static bool           sosPending      = false;
static unsigned long  sosWaitStartMs  = 0;
const  unsigned long  SOS_GPS_WAIT_MS = 2500UL;
const  uint32_t       SOS_HOLD_MS     = 2000UL;   // shared with checkButton()'s hold detection

// ── GPS TX payload and phone GPS cache ───────────────────────────────────────
static char gpsTxPayload[128]  = {};
static char phoneGpsLatBuf[20] = {};
static char phoneGpsLngBuf[20] = {};
static char phoneGpsAltBuf[12] = {};
static char phoneGpsSpdBuf[12] = {};
static char phoneGpsHdgBuf[12] = {};
static unsigned long gpsReqSentMs         = 0;
static unsigned long gpsReqDeferredSendMs = 0;
static unsigned long gpsDisplayClearMs    = 0;

// ── Signal / TX tracking ─────────────────────────────────────────────────────
static int           lastSignalPct        = -1;
static int           lastTxResult         = -1;
static unsigned long lastTxMs             = 0;
static unsigned long lastHomeRefreshMs    = 0;
const  unsigned long HOME_REFRESH_MS      = 5000UL;

// ── Message display ───────────────────────────────────────────────────────────
static bool          messagePending          = false;
static bool          emergencyDisplayPending = false;
static unsigned long sosAckUntilMs          = 0;    // epoch when SOS ACK screen auto-dismisses (0 = not active)
const  unsigned long SOS_ACK_DISPLAY_MS     = 10000UL;

// Beep requests from duck.run() callbacks are deferred here and executed in
// loop() after duck.run() returns, avoiding recursive duck.run() deadlocks.
static struct { int times; int onMs; int offMs; } gBeepReq = {};

// ── Battery ───────────────────────────────────────────────────────────────────
static unsigned long lastBattMs  = 0;

// ── Per-duck GPS cache ────────────────────────────────────────────────────────
struct DuckGps { float lat; float lng; unsigned long tsMs; };
static std::map<String, DuckGps> duckGpsCache;
constexpr unsigned long DUCK_GPS_TTL_MS = 300000UL;   // 5 minutes

// ── Custom discovery topics (BEACON / BEACON_ACK) ────────────────────────────
static const uint8_t  TOPIC_BEACON      = 27;
static const uint8_t  TOPIC_BEACON_ACK  = 28;
static volatile bool  beaconAckPending  = false;
static char           beaconAckPayload[80] = {};
static unsigned long  beaconAckDeferMs  = 0;

// ── Duck instance ─────────────────────────────────────────────────────────────
MamaDuck duck(DUCK_NAME);
static bool setupOK = false;
static int  counter = 1;
static char idBuf[12];    // "ID:IBRAHIM1\0" header string shown on screen

// ── Function declarations ─────────────────────────────────────────────────────
void handleDuckData(CdpPacket packet);
void displayMessage(String msg);
void displayAnnouncement(const String& msg);
void displayHome();
void displayID();
void displayBatt();
void flashLED();
void handleFrame(const String& line);
void broadcast(const String& frame);
void sendBattery();
void handleSOS(const String& body);
void handleMsg(const String& body);
void handleMamaTalk(const String& body);
bool sendMamaTalk(const String& targetId, const String& msg, const String& mid = "");
String extractField(const String& body, const String& key);
void handleGps(const String& body);
void blinkLed(int times);
void beepBuzzer(int times, int onMs = 100, int offMs = 100);
bool sendEmergency(String lat = "", String lng = "", String alt = "",
                   String spd = "", String hdg = "", bool gpsFromPhone = false);
static float readVbat();
static int batteryPercent(float vbat);
static bool isPhoneConnected();

// ── SVC dispatch & fault handling ────────────────────────────────────────────
// Strong SVC_Handler overrides the BSP's weak vPortSVCHandler (port.c patched).
// Routes: MSP path / SVC 0 → FreeRTOS first-task restore (vPortStartFirstTask)
//         PSP path, SVC >0 → SoftDevice handler at *(0x102C)
// After sd_softdevice_enable(), VTOR=0x00000000 (MBR routing); all subsequent
// SD SVC calls go MBR→SD directly — no RAM VT needed.
extern "C" { extern void * volatile pxCurrentTCB; }

extern "C" __attribute__((naked, used)) void SVC_Handler(void) {
    __asm volatile (
        "tst   lr, #0x04         \n"   // EXC_RETURN bit2: 0=MSP, 1=PSP
        "beq   1f                \n"   // MSP → FreeRTOS first-task start
        "mrs   r0, psp           \n"
        "ldr   r1, [r0, #0x18]   \n"   // stacked PC
        "ldrb  r2, [r1, #-2]     \n"   // SVC immediate byte
        "cmp   r2, #0            \n"
        "bne   2f                \n"   // SVC>0 → SD
        "1:                      \n"
        "ldr   r3, =pxCurrentTCB \n"
        "ldr   r1, [r3]          \n"
        "ldr   r0, [r1]          \n"
        "ldmia r0!, {r4-r11, r14}\n"
        "msr   psp, r0           \n"
        "isb                     \n"
        "mov   r0, #0            \n"
        "msr   basepri, r0       \n"
        "bx    r14               \n"
        "2:                      \n"
        "ldr   r1, =0x102C       \n"
        "ldr   r1, [r1]          \n"
        "bx    r1               \n"
    );
}

// Override BSP's HardFault_Handler (NVIC_SystemReset) with SOS LED blinks
// so a fault is visible without a serial monitor.  debug.cpp patched weak.
extern "C" void HardFault_Handler(void) {
    NRF_P1->DIRSET = (1u << 1);
    while (true) {
        for (int i=0;i<3;i++){NRF_P1->OUTSET=(1u<<1);for(volatile uint32_t d=0;d<1920000u;d++){}NRF_P1->OUTCLR=(1u<<1);for(volatile uint32_t d=0;d<1920000u;d++){}}
        for(volatile uint32_t d=0;d<3840000u;d++){}
        for (int i=0;i<3;i++){NRF_P1->OUTSET=(1u<<1);for(volatile uint32_t d=0;d<6400000u;d++){}NRF_P1->OUTCLR=(1u<<1);for(volatile uint32_t d=0;d<3200000u;d++){}}
        for(volatile uint32_t d=0;d<3840000u;d++){}
        for (int i=0;i<3;i++){NRF_P1->OUTSET=(1u<<1);for(volatile uint32_t d=0;d<1920000u;d++){}NRF_P1->OUTCLR=(1u<<1);for(volatile uint32_t d=0;d<1920000u;d++){}}
        for(volatile uint32_t d=0;d<9600000u;d++){}
    }
}

static void ble_on_connect(uint16_t /*conn_handle*/) {
    blePhoneSeen = true;
    usbConnectDisplayPending = true;
    Serial.println("[BLE] connected"); Serial.flush();
    broadcast(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
    sendBattery();
}

static void ble_on_disconnect(uint16_t /*conn_handle*/, uint8_t reason) {
    blePhoneSeen = false;
    usbDisconnectDisplayPending = true;
    bleInBuf = "";
    Serial.printf("[BLE] disconnected reason=0x%02X\n", reason); Serial.flush();
}

static void ble_uart_rx_cb(uint16_t /*conn_handle*/) {
    // Called from BLE task context — buffer the line, set flag for loop().
    while (bleuart.available()) {
        char c = (char)bleuart.read();
        if (c == '\n') {
            if (bleInBuf.length() > 0) {
                bleInBuf.toCharArray(bleRxLine, sizeof(bleRxLine));
                bleRxPending = true;
            }
            bleInBuf = "";
        } else if (c != '\r') {
            if (bleInBuf.length() < (sizeof(bleRxLine) - 2))
                bleInBuf += c;
        }
    }
}

static void bleSendLine(const String& line) {
    if (!blePhoneSeen || !Bluefruit.connected()) return;
    String payload = line.endsWith("\n") ? line : line + "\n";
    bleuart.write((const uint8_t*)payload.c_str(), payload.length());
}

static bool bleIsAdvertising() {
    ble_gap_addr_t addr;
    return (sd_ble_gap_adv_addr_get(0, &addr) == NRF_SUCCESS);
}

// Called last in setup() — after Serial, display, GPS, CDP/LoRa are all stable.
static void setupBLE() {
    // Disable ConnLed: D12 is the BUZZER on Wio Tracker L1, not an LED.
    // Without this, _startConnLed() fires a 4 Hz FreeRTOS timer that clicks the buzzer.
    Bluefruit.autoConnLed(false);
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    bool ble_ok = Bluefruit.begin(1, 0);
    // Enable DC-DC — must be after begin() (SD must be running first).
    // Reduces coil-whine from RADIO current spikes during advertising.
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    if (!ble_ok) {
        dspStatus("BLE FAIL", "begin()");
        Serial.println("[BLE] begin() FAILED"); Serial.flush();
        return;
    }
    Serial.println("[BLE] begin() OK"); Serial.flush();

    // ── PPCP + name ──────────────────────────────────────────────────────────
    ble_gap_conn_params_t ppcp;
    ppcp.min_conn_interval = BLE_MIN_CONN_INTERVAL;
    ppcp.max_conn_interval = BLE_MAX_CONN_INTERVAL;
    ppcp.slave_latency     = BLE_SLAVE_LATENCY;
    ppcp.conn_sup_timeout  = BLE_CONN_SUP_TIMEOUT;
    sd_ble_gap_ppcp_set(&ppcp);
    Bluefruit.setTxPower(4);
    Bluefruit.setName(DUCK_NAME);

    // ── callbacks + NUS UART ─────────────────────────────────────────────────
    Bluefruit.Periph.setConnectCallback(ble_on_connect);
    Bluefruit.Periph.setDisconnectCallback(ble_on_disconnect);
    bleuart.begin();
    bleuart.setRxCallback(ble_uart_rx_cb);

    // ── advertising ──────────────────────────────────────────────────────────
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    // Fast interval 160×0.625ms=100ms (was 20ms) — reduces coil-whine tick rate
    // while still being discoverable. Slow interval 244×0.625ms≈152ms unchanged.
    // Fast timeout 10s (was 30s) — switches to slow mode sooner.
    Bluefruit.Advertising.setInterval(160, 244);
    Bluefruit.Advertising.setFastTimeout(10);

    if (!Bluefruit.Advertising.start(0)) {
        Serial.println("[BLE] Advertising.start() FAILED"); Serial.flush();
        dspStatus("BLE FAIL", "adv.start()");
        return;
    }
    bleAdvertisingStarted = true;
    lastBleHealthMs = millis();
    sd_power_gpregret_clr(0, 0xFF);
    dspStatus("ADV STARTED!", DUCK_NAME);
    Serial.println(String("[BLE] advertising as '") + DUCK_ID_BUF + "'"); Serial.flush();
}

// ── Busy-wait LED blink helper ─────────────────────────────────────────────
// Works without FreeRTOS tick or any library.  LED is D11 = P1.01.
#define BLINK_LED(n) do { \
    NRF_P1->DIRSET = (1u<<1); \
    for(int _b=0;_b<(n);_b++){ \
        NRF_P1->OUTSET=(1u<<1); for(volatile uint32_t _d=0;_d<9600000u;_d++){} \
        NRF_P1->OUTCLR=(1u<<1); for(volatile uint32_t _d=0;_d<9600000u;_d++){} \
    } \
    for(volatile uint32_t _d=0;_d<32000000u;_d++){} \
} while(0)

// ── Battery ADC ───────────────────────────────────────────────────────────────
static float readVbat() {
    // Drive BAT_READ HIGH to enable the battery voltage-divider (active HIGH on
    // Seeed nRF52840 designs — a LOW gate keeps the switch open).
    pinMode(BAT_READ, OUTPUT);
    digitalWrite(BAT_READ, HIGH);
    delay(5);
    // analogReadResolution(ADC_RESOLUTION) is called in setup(), so
    // analogRead() returns a 14-bit value (0–16383).
    float raw  = (float)analogRead(PIN_VBAT);
    float vbat = raw / (float)((1 << ADC_RESOLUTION) - 1) * AREF_VOLTAGE * ADC_MULTIPLIER;
    // Return BAT_READ pin to input (Hi-Z) to save power.
    pinMode(BAT_READ, INPUT);
    return vbat;
}

static int batteryPercent(float vbat) {
    // LiPo: 4.2 V = 100%, 3.5 V = 0%.  Adjust thresholds as needed.
    float pct = (vbat - 3.5f) / (4.2f - 3.5f) * 100.0f;
    return (int)constrain(pct, 0.0f, 100.0f);
}

// Draw a live progress bar while the SOS button is held, so the user gets
// clear visual (not just audible) confirmation the hold is registering and
// can see roughly how much longer is needed. Throttled by the caller so this
// doesn't hammer the bit-banged SW-I2C bus every loop() iteration.
static void showHoldProgress(uint32_t heldMs) {
    dspBegin();
    dspStrRight(0, idBuf);
    dspStrCenter(14, TXT_HOLD_FOR_SOS);

    const int barX = 14, barY = 30, barW = 100, barH = 14;
    display.drawFrame(barX, barY, barW, barH);
    uint32_t clampedMs = (heldMs > SOS_HOLD_MS) ? SOS_HOLD_MS : heldMs;
    int fillW = (int)((uint32_t)(barW - 4) * clampedMs / SOS_HOLD_MS);
    if (fillW > 0) display.drawBox(barX + 2, barY + 2, fillW, barH - 4);

    dspEnd();
}

// ── Button debouncer ──────────────────────────────────────────────────────────
// (enum BtnEvent declared earlier near top of file)

static BtnEvent checkButton() {
    static bool     wasDown      = false;
    static uint32_t pressStartMs = 0;
    static uint8_t  clickCount   = 0;
    static uint32_t lastReleaseMs = 0;
    static bool     holdFired    = false;
    static uint8_t  holdBeepsFired = 0;   // how many hold-progress beeps fired this press
    static bool     progressShown  = false; // true once the on-screen hold bar has been drawn this press
    static uint32_t lastProgressDrawMs = 0;

    const uint32_t HOLD_MS        = SOS_HOLD_MS;
    const uint32_t CLICK_GAP      = 400;   // max ms between clicks in a multi-click burst
    const uint32_t PROGRESS_DELAY = 200;   // ms held before showing the bar (avoids flicker on quick clicks)

    bool btnDown = (digitalRead(CANCEL_BUTTON_PIN) == LOW);  // active LOW

    if (btnDown && !wasDown) {
        wasDown      = true;
        pressStartMs = millis();
        holdFired    = false;
        holdBeepsFired = 0;
        progressShown  = false;
    }
    // Live feedback while holding, so the user knows the SOS hold is being
    // registered and roughly how much longer to keep pressing (helps avoid
    // releasing too early, or wondering if the button is unresponsive).
    if (wasDown && btnDown && !holdFired) {
        uint32_t heldMs = millis() - pressStartMs;
        if (holdBeepsFired < 1 && heldMs >= 500)  { beepBuzzer(1, 40, 0); holdBeepsFired = 1; }
        if (holdBeepsFired < 2 && heldMs >= 1000) { beepBuzzer(1, 40, 0); holdBeepsFired = 2; }
        if (holdBeepsFired < 3 && heldMs >= 1500) { beepBuzzer(2, 40, 40); holdBeepsFired = 3; }

        if (heldMs >= PROGRESS_DELAY && (!progressShown || millis() - lastProgressDrawMs >= 100)) {
            lastProgressDrawMs = millis();
            progressShown      = true;
            showHoldProgress(heldMs);
        }
    }
    // Detect 2-second hold while button is still pressed (fast path).
    if (wasDown && btnDown && !holdFired && (millis() - pressStartMs >= HOLD_MS)) {
        holdFired  = true;
        wasDown    = false;
        clickCount = 0;
        return BTN_HOLD_2S;
    }
    // Button released — also check for hold on release in case polling was
    // delayed by a beep (the button may have been released during the block).
    if (!btnDown && wasDown) {
        wasDown       = false;
        lastReleaseMs = millis();
        if (!holdFired && (millis() - pressStartMs >= HOLD_MS)) {
            clickCount = 0;
            return BTN_HOLD_2S;
        }
        if (!holdFired) {
            clickCount++;
            beepBuzzer(1, 25, 0);   // immediate tick per click so the user can
                                    // self-correct a multi-click gesture in progress
        }
        // Released before the hold completed — restore whatever the screen
        // showed before we interrupted it with the progress bar.
        if (progressShown) {
            progressShown = false;
            displayHome();
        }
    }
    // Evaluate click burst after the inter-click silence window expires
    if (!btnDown && !wasDown && clickCount > 0 && (millis() - lastReleaseMs >= CLICK_GAP)) {
        uint8_t n  = clickCount;
        clickCount = 0;
        if      (n == 1) return BTN_SINGLE;
        else if (n == 2) return BTN_DOUBLE;
        else if (n == 3) return BTN_TRIPLE;
        else if (n >= 4) return BTN_QUAD;
    }
    return BTN_NONE;
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    // 1 LED blink = firmware is alive, setup() entered (visible before Serial init).
    // If you see no LED activity at all after reset, the firmware is not running.
    BLINK_LED(1);

    // USB serial (debug / phone comms)
    Serial.begin(115200);

    // Show display content immediately — before waiting for USB CDC so there
    // is always visual feedback even on fast crash/reset loops.
    initDisplay();
    dspStatus("Booting...", DUCK_NAME);

    Serial.println("[BOOT] Seeed Wio Tracker L1 Pro MamaDuck"); Serial.flush();
    Serial.println("[SETUP] USB serial ready"); Serial.flush();

    // ADC resolution — must be called before any analogRead().
    // ADC_RESOLUTION = 14 is defined in variant.h; the BSP defaults to 10 if
    // this call is omitted.  14-bit gives full-scale 16383 (0x3FFF).
    analogReadResolution(ADC_RESOLUTION);

    // LED + Button + Buzzer
    pinMode(PIN_LED1,     OUTPUT);
    digitalWrite(PIN_LED1, LOW);
    pinMode(12,           OUTPUT);              // D12 = Buzzer (active HIGH)
    digitalWrite(12,      LOW);
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);   // active LOW

    // GPS — wake the L76KB before starting Serial1
    pinMode(PIN_GPS_STANDBY, OUTPUT);
    digitalWrite(PIN_GPS_STANDBY, HIGH);   // STDBY_N high = active
    Serial1.begin(GPS_BAUDRATE);
    Serial.println("[GPS] Serial1 started at " + String(GPS_BAUDRATE) + " baud"); Serial.flush();

    // Enable all three GNSS constellations for faster and more reliable signal acquisition.
    // The L76KB default is GPS-only; adding GLONASS + BeiDou roughly triples visible satellites.
    // PMTK353: GPS(1) + GLONASS(1) + BeiDou(1) + Galileo(0) + NAVIC(0) — checksum 0x2A verified.
    delay(100);  // brief settle time after module power-on
    Serial1.println("$PMTK353,1,1,1,0,0*2A");
    delay(50);

    // initDisplay() + dspStatus("Booting...") were moved to before the Serial
    // wait at the top of setup() so the display always shows content on boot.

    // ── CDP init (LoRa / routing / storage) ─────────────────────────────────
    dspStatus("CDP init", DUCK_NAME);
    if (duck.setupWithDefaults() != DUCK_ERR_NONE) {
        BLINK_LED(10);  // 10 blinks = CDP setup failed
        Serial.println("[MAMA] Failed to setup MamaDuck"); Serial.flush();
        return;
    }
    duck.goPublic();
    Serial.println("[MAMA] Network state: PUBLIC"); Serial.flush();
    duck.onReceiveDuckData(handleDuckData);
    setupOK = true;
    snprintf(idBuf, sizeof(idBuf), "ID:%s", DUCK_NAME);
    BLINK_LED(5);   // 5 blinks = CDP fully initialized
    dspStatus("CDP OK", DUCK_NAME);
    Serial.println("[MAMA] CDP OK"); Serial.flush();

    // Re-init display after CDP/LoRa are stable (before BLE, which may
    // also disturb I2C — setupBLE() does a second re-init after begin()).
    if (gDisplayOk) {
        display.begin();
        display.setContrast(255);
        display.setPowerSave(0);
        display.setFont(u8g2_font_6x10_tf);
        dspStatus("CDP Ready", DUCK_NAME);
    }
    BLINK_LED(3);

    // BLE init goes last — same order as MeshCore (radio/filesystem first,
    // BLE last).  setupBLE() calls Bluefruit.begin() and starts advertising.
    setupBLE();

    // Re-assert GPS wakeup — BLE init can take several hundred ms.
    digitalWrite(PIN_GPS_STANDBY, HIGH);

    // Re-configure button — defensive in case BLE/SD peripheral init disturbed it.
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);

    Serial.println("[MAMA] Setup complete"); Serial.flush();
    Serial.println(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
    sendBattery();
}

// ── Main loop ─────────────────────────────────────────────────────────────────
void loop() {
    // Diagnostic heartbeat — always runs, even if setup() failed.
    // Lets us see the reason via serial without missing the one-shot setup() log.
    static unsigned long lastDiagMs = 0;
    if (!setupOK && millis() - lastDiagMs >= 2000UL) {
        lastDiagMs = millis();
        Serial.println("[DIAG] setupOK=false — setup() did not complete");
    }
    // Diagnostic: 7 fast blinks at first loop() entry — visible even if setupOK=false.
    static bool loopEntryBlinked = false;
    if (!loopEntryBlinked) {
        loopEntryBlinked = true;
        for (int _i = 0; _i < 7; _i++) {
            digitalWrite(PIN_LED1, HIGH); delay(50);
            digitalWrite(PIN_LED1, LOW);  delay(50);
        }
        delay(400);
    }
    if (!setupOK) return;

    // ── First-run display init ────────────────────────────────────────────────
    // Deferred here so the [DISP] log appears AFTER serial is connected.
    // I2C bus recovery (9 SCL clocks) frees any slave holding SDA low.
    // Wait up to 5 s for the serial monitor to attach so [DISP] logs are visible.
    static unsigned long loopFirstMs = 0;
    if (loopFirstMs == 0) loopFirstMs = millis();
    if (!Serial && millis() - loopFirstMs < 500) return;

    static bool displayProbed = false;
    if (!displayProbed) {
        displayProbed = true;
        // Display was already initialised in setup(); just show the home screen.
        if (gDisplayOk) {
            dspBegin();
            dspStrCenter(24, DUCK_NAME);
            dspStrCenter(36, "CDP MAMADUCKLING");
            dspStrCenter(48, "nRF52840 / SX1262");
            dspEnd();
            delay(1000);
            displayHome();
        }
    }

    // ── Display deferred rendering ─────────────────────────────────────────────

    if (phoneGpsDisplayPending) {
        phoneGpsDisplayPending = false;
        dspPowerSave(0);
        dspBegin();
        dspStr(0, 0, ("BATT:" + String(batteryPercent(readVbat())) + "%").c_str());
        dspStrRight(0, idBuf);
        if (phoneGpsNoFix) {
            dspStrCenter(26, TXT_PHONE_GPS);
            dspStrCenter(38, TXT_NO_SIGNAL);
            dspEnd();
            delay(2000);
        } else {
            dspStr(0, 14, gpsLoraOk ? TXT_GPS_SENT_OK : TXT_GPS_SEND_FAIL);
            dspStr(0, 28, ("LAT:" + String(phoneGpsLatBuf)).c_str());
            dspStr(0, 40, ("LNG:" + String(phoneGpsLngBuf)).c_str());
            dspStr(0, 52, TXT_SRC_PHONE);
            dspEnd();
            delay(3000);
        }
        dspPowerSave(1);
    }

    // USB connected / disconnected splashes.
    if (usbConnectDisplayPending) {
        usbConnectDisplayPending = false;
        dspPowerSave(0);
        dspBegin();
        dspStrRight(0, idBuf);
        dspStrCenter(26, TXT_USB_SERIAL);
        dspStrCenter(38, TXT_CONNECTED_BANG);
        dspEnd();
        delay(2000);
        displayHome();
    }
    if (usbDisconnectDisplayPending) {
        usbDisconnectDisplayPending = false;
        dspPowerSave(0);
        displayEnabled = true;
        dspBegin();
        dspStrRight(0, idBuf);
        dspStrCenter(26, TXT_USB_SERIAL);
        dspStrCenter(38, TXT_DISCONNECTED);
        dspEnd();
        delay(2000);
        displayHome();
    }

    // SOS acknowledgement from operator — show for SOS_ACK_DISPLAY_MS then auto-return home.
    if (sosAckDisplayPending) {
        sosAckDisplayPending = false;
        displayEnabled = true;
        dspPowerSave(0);
        dspBegin();
        dspStrRight(0, idBuf);
        dspStrCenter(16, TXT_SOS_RECEIVED);
        dspStrCenter(28, TXT_HELP_BEING);
        dspStrCenter(40, TXT_SENT);
        dspEnd();
        blinkLed(3);
        messagePending = false;
        sosAckUntilMs  = millis() + SOS_ACK_DISPLAY_MS;
    }

    // Auto-dismiss SOS ACK screen after timeout.
    if (sosAckUntilMs > 0 && millis() >= sosAckUntilMs) {
        sosAckUntilMs = 0;
        dspPowerSave(0);
        displayHome();
    }

    // Auto-refresh home screen.
    if (displayEnabled && !phoneGpsDisplayPending
        && !usbConnectDisplayPending && !messagePending && !emergencyDisplayPending
        && sosAckUntilMs == 0
        && (millis() - lastHomeRefreshMs >= HOME_REFRESH_MS)) {
        lastHomeRefreshMs = millis();
        float rawRssi = lora.getRSSI();
        if (rawRssi < 0.0f) {
            float normRssi = constrain((rawRssi       - RSSI_MIN) / (RSSI_MAX - RSSI_MIN), 0.0f, 1.0f);
            float normSnr  = constrain((lora.getSNR() - SNR_MIN)  / (SNR_MAX  - SNR_MIN),  0.0f, 1.0f);
            lastSignalPct  = (int)(((normRssi + normSnr) / 2.0f) * 100.0f);
        }
        displayHome();
    }

    // ── Button ────────────────────────────────────────────────────────────────
    BtnEvent btn = checkButton();

    if (btn == BTN_HOLD_2S) {
        // Hardware-button SOS — request GPS from phone if we don't have a fix.
        String gpsLat, gpsLng, gpsAlt, gpsSpd, gpsHdg;
        bool gotGps = false;

        if (tinyGps.location.isValid() && tinyGps.location.age() < 5000) {
            gpsLat = String(tinyGps.location.lat(), 6);
            gpsLng = String(tinyGps.location.lng(), 6);
            if (tinyGps.altitude.isValid()) gpsAlt = String(tinyGps.altitude.meters(), 1);
            if (tinyGps.speed.isValid())    gpsSpd = String(tinyGps.speed.kmph(), 1);
            if (tinyGps.course.isValid())   gpsHdg = String(tinyGps.course.deg(), 1);
            gotGps = true;
        }

        if (gotGps) {
            // Already have a local fix — send immediately, no need to wait.
            dspBegin();
            dspStrRight(0, idBuf);
            displayBatt();
            dspStrCenter(22, TXT_SENDING);
            dspStrCenter(34, TXT_EMERGENCY_SIGNAL_DOTS);
            dspEnd();
            sendEmergency(gpsLat, gpsLng, gpsAlt, gpsSpd, gpsHdg, /* gpsFromPhone= */ false);
        } else if (phoneGpsLatBuf[0] != '\0') {
            // We already have a cached phone fix from earlier — use it now.
            dspBegin();
            dspStrRight(0, idBuf);
            displayBatt();
            dspStrCenter(22, TXT_SENDING);
            dspStrCenter(34, TXT_EMERGENCY_SIGNAL_DOTS);
            dspEnd();
            sendEmergency(String(phoneGpsLatBuf), String(phoneGpsLngBuf),
                          phoneGpsAltBuf[0] ? String(phoneGpsAltBuf) : "",
                          phoneGpsSpdBuf[0] ? String(phoneGpsSpdBuf) : "",
                          phoneGpsHdgBuf[0] ? String(phoneGpsHdgBuf) : "",
                          /* gpsFromPhone= */ true);
        } else if (isPhoneConnected()) {
            // No fix yet — ask the phone and continue asynchronously below
            // (see "Deferred SOS" block) so we don't block button polling
            // or duck.run(). Single-click cancels this while it's pending.
            dspBegin();
            dspStrRight(0, idBuf);
            dspStrCenter(22, TXT_REQUESTING_GPS);
            dspStrCenter(34, TXT_FROM_PHONE_DOTS);
            dspStrCenter(46, TXT_CLICK_TO_CANCEL);
            dspEnd();
            broadcast("CDK:GPSREQ");
            sosPending     = true;
            sosWaitStartMs = millis();
        } else {
            // No local fix, no phone connected — send anyway, but make sure
            // the user sees clearly that no location was included.
            dspBegin();
            dspStrRight(0, idBuf);
            displayBatt();
            dspStrCenter(22, TXT_SENDING);
            dspStrCenter(34, TXT_EMERGENCY_SIGNAL_DOTS);
            dspEnd();
            sendEmergency("", "", "", "", "", /* gpsFromPhone= */ false);
        }
    }

    if (btn == BTN_SINGLE) {
        if (sosPending) {
            // Cancel a pending SOS that's still waiting on a phone GPS reply.
            sosPending = false;
            beepBuzzer(1, 60, 0);
            dspPowerSave(0);
            displayEnabled = true;
            displayHome();
        } else if (sosAckUntilMs > 0) {
            sosAckUntilMs = 0;
            dspPowerSave(0);
            displayHome();
        } else if (emergencyDisplayPending) {
            emergencyDisplayPending = false;
            displayEnabled = true;
            dspPowerSave(0);
            displayHome();
        } else if (messagePending) {
            messagePending = false;
            displayEnabled = true;
            dspPowerSave(0);
            displayHome();
        } else {
            displayEnabled = !displayEnabled;
            if (displayEnabled) {
                dspPowerSave(0);
                displayHome();
            } else {
                dspPowerSave(1);
            }
        }
    }

    // Double-click: Roger acknowledgement — moved from triple-click since
    // this is the more time-critical rescue-coordination action and
    // deserves the fewer-clicks slot. The old double-click battery-send
    // feature was removed: battery is already auto-broadcast on BLE
    // connect (ble_on_connect()) and periodically over USB, so a manual
    // send added no information the phone didn't already have.
    if (btn == BTN_DOUBLE) {
        duck.sendData(topics::status, std::string("MSG,SRC:DEVICE,TEXT:Roger"));
        broadcast("CDK:ACK,ID:ROGER");
        dspPowerSave(0);
        displayEnabled = true;
        dspBegin();
        dspStrRight(0, idBuf);
        dspStrCenter(28, TXT_ROGER_SENT);
        dspEnd();
        delay(2000);
        displayHome();
    }

    // Triple-click: GPS/date-time pages — moved from quadruple-click now
    // that only three gestures (single/double/triple) are used.
    if (btn == BTN_TRIPLE) {
        dspPowerSave(0);
        displayEnabled = true;
        dspBegin();
        dspStrRight(0, idBuf);
        displayBatt();
        if (tinyGps.location.isValid()) {
            dspStr(0, 14, ("LAT:" + String(tinyGps.location.lat(), 5)).c_str());
            dspStr(0, 26, ("LNG:" + String(tinyGps.location.lng(), 5)).c_str());
            dspStr(0, 38, ("SATS:" + String(tinyGps.satellites.value())).c_str());
            dspStr(0, 50, ("AGE:" + String(tinyGps.location.age()) + "ms").c_str());
            dspEnd();
            delay(2500);

            // Page 2: UTC+8 date/time
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
            dspBegin();
            dspStrRight(0, idBuf);
            displayBatt();
            if (tinyGps.date.isValid() && tinyGps.time.isValid()) {
                char dateBuf[20], timeBuf[20];
                snprintf(dateBuf, sizeof(dateBuf), "DATE:%04d/%02d/%02d", y, mo, d);
                snprintf(timeBuf, sizeof(timeBuf), "TIME:%02d:%02d:%02d", h, mi, sc);
                dspStr(0, 14, dateBuf);
                dspStr(0, 26, timeBuf);
                dspStr(0, 38, "(GMT+8 / UTC+8)");
            } else {
                dspStrCenter(26, TXT_DATE_TIME);
                dspStrCenter(38, TXT_NO_SIGNAL);
            }
            dspEnd();
            delay(2500);
        } else {
            dspStrCenter(28, gpsModuleDetected ? TXT_GPS_MODULE_ACTIVE : TXT_GPS_NO_MODULE);
            dspStrCenter(40, gpsModuleDetected ? TXT_WAITING_SIGNAL_DOTS : "(Serial1)");
            dspEnd();
            delay(2500);
        }
        displayHome();
    }

    // ── USB discovery / incoming ───────────────────────────────────────────────
    {
        static unsigned long lastUsbAnnounceMs = 0;
        if (usbPhoneSeen && lastUsbRxMs > 0 && millis() - lastUsbRxMs > USB_IDLE_TIMEOUT_MS) {
            usbPhoneSeen             = false;
            lastUsbAnnounceMs        = 0;
            usbDisconnectDisplayPending = true;
        }
        if (!usbPhoneSeen && millis() - lastUsbAnnounceMs >= 3000UL) {
            Serial.println(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
            sendBattery();
            lastBattMs        = millis();
            lastUsbAnnounceMs = millis();
        }
    }

    // ── BLE incoming (dispatched from ble_uart_rx_cb ISR-context) ──────────────
    if (bleRxPending) {
        String line = String(bleRxLine);
        bleRxPending = false;
        if (line.startsWith("CDK:")) handleFrame(line);
    }

    // BLE advertising watchdog — mirrors MeshCore's health-check logic.
    if (bleAdvertisingStarted && !Bluefruit.connected() &&
        millis() - lastBleHealthMs >= BLE_HEALTH_INTERVAL_MS) {
        lastBleHealthMs = millis();
        if (!bleIsAdvertising()) {
            Serial.println("[BLE] watchdog: restarting advertising"); Serial.flush();
            Bluefruit.Advertising.start(0);
        }
    }

    while (Serial.available()) {
        lastUsbRxMs = millis();
        char c = Serial.read();
        if (c == '\n') {
            if (usbInBuf.startsWith("CDK:")) {
                if (!usbPhoneSeen) usbConnectDisplayPending = true;
                usbPhoneSeen = true;
            }
            handleFrame(usbInBuf);
            usbInBuf = "";
        } else if (c != '\r') {
            usbInBuf += c;
        }
    }

    // Periodic battery update every 60 s.
    if (millis() - lastBattMs >= 60000UL) {
        sendBattery();
        lastBattMs = millis();
        if (displayEnabled) displayHome();
    }

    // Feed GPS NMEA into TinyGPSPlus.
    while (Serial1.available()) {
        char c = Serial1.read();
        if (!gpsModuleDetected) {
            gpsModuleDetected = true;
            Serial.println("[GPS] Module detected");
        }
        tinyGps.encode(c);
    }
    if (!gpsFix && tinyGps.location.isValid()) {
        gpsFix = true;
        Serial.printf("[GPS] Fix: lat=%.6f lng=%.6f sats=%u\n",
                      tinyGps.location.lat(), tinyGps.location.lng(),
                      tinyGps.satellites.value());
    }

    duck.run();

    // Execute any beep deferred from duck.run() callbacks (avoids recursive duck.run()).
    if (gBeepReq.times > 0) {
        int t = gBeepReq.times, on = gBeepReq.onMs, off = gBeepReq.offMs;
        gBeepReq = {};
        beepBuzzer(t, on, off);
    }

    // ── Deferred GPS LoRa TX ──────────────────────────────────────────────────
    if (gpsTxPending) {
        gpsTxPending = false;
        int result = duck.sendData(topics::gps, std::string(gpsTxPayload));
        gpsLoraOk   = (result == 0);
        Serial.printf("[GPS] Deferred TX %s: %s\n", gpsLoraOk ? "OK" : "FAILED", gpsTxPayload);
    }

    // ── Deferred BEACON_ACK TX ────────────────────────────────────────────────
    if (beaconAckPending && beaconAckDeferMs == 0) {
        beaconAckDeferMs = millis() + 350;
    }
    if (beaconAckDeferMs > 0 && millis() >= beaconAckDeferMs && !gpsTxPending) {
        beaconAckDeferMs = 0;
        beaconAckPending = false;
        duck.sendData(TOPIC_BEACON_ACK, std::string(beaconAckPayload), BROADCAST_DUID);
        Serial.printf("[BEACON] ACK TX: %s\n", beaconAckPayload);
    }

    // ── Deferred display clear after GPS request ──────────────────────────────
    if (gpsDisplayClearMs > 0 && millis() >= gpsDisplayClearMs) {
        gpsDisplayClearMs = 0;
        dspPowerSave(1);
    }

    // ── Deferred SOS: waiting (non-blocking) for a phone GPS reply ───────────
    // Triggered from BTN_HOLD_2S above when no local fix was available yet.
    // Cancelled by a single click (see BTN_SINGLE handling above).
    if (sosPending) {
        if (phoneGpsLatBuf[0] != '\0') {
            sosPending = false;
            dspBegin();
            dspStrRight(0, idBuf);
            displayBatt();
            dspStrCenter(22, TXT_SENDING);
            dspStrCenter(34, TXT_EMERGENCY_SIGNAL_DOTS);
            dspEnd();
            sendEmergency(String(phoneGpsLatBuf), String(phoneGpsLngBuf),
                          phoneGpsAltBuf[0] ? String(phoneGpsAltBuf) : "",
                          phoneGpsSpdBuf[0] ? String(phoneGpsSpdBuf) : "",
                          phoneGpsHdgBuf[0] ? String(phoneGpsHdgBuf) : "",
                          /* gpsFromPhone= */ true);
        } else if (millis() - sosWaitStartMs >= SOS_GPS_WAIT_MS) {
            sosPending = false;
            dspBegin();
            dspStrRight(0, idBuf);
            displayBatt();
            dspStrCenter(22, TXT_SENDING);
            dspStrCenter(34, TXT_EMERGENCY_SIGNAL_DOTS);
            dspEnd();
            sendEmergency("", "", "", "", "", /* gpsFromPhone= */ false);
        }
    }

    // ── Deferred CDK:GPSREQ dispatch ─────────────────────────────────────────
    if (gpsReqDeferredSendMs > 0 && millis() >= gpsReqDeferredSendMs) {
        gpsReqDeferredSendMs = 0;
        if (isPhoneConnected() && phoneGpsLatBuf[0] == '\0') {
            broadcast("CDK:GPSREQ");
            gpsReqSentMs = millis();
        } else if (phoneGpsLatBuf[0] != '\0' && !gpsTxPending) {
            char gpsBuf[128];
            snprintf(gpsBuf, sizeof(gpsBuf), "GPS,SRC:PHONE,LAT:%s,LNG:%s",
                     phoneGpsLatBuf, phoneGpsLngBuf);
            if (phoneGpsAltBuf[0] != '\0')
                strncat(gpsBuf, (",ALT:" + String(phoneGpsAltBuf)).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1);
            if (phoneGpsSpdBuf[0] != '\0')
                strncat(gpsBuf, (",SPD:" + String(phoneGpsSpdBuf)).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1);
            if (phoneGpsHdgBuf[0] != '\0')
                strncat(gpsBuf, (",HDG:" + String(phoneGpsHdgBuf)).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1);
            char battSuffix[16];
            snprintf(battSuffix, sizeof(battSuffix), ",BATT:%d", batteryPercent(readVbat()));
            strncat(gpsBuf, battSuffix, sizeof(gpsBuf) - strlen(gpsBuf) - 1);
            strncpy(gpsTxPayload, gpsBuf, sizeof(gpsTxPayload) - 1);
            gpsTxPending = true;
        }
    }

    // GPS request timeout fallback.
    if (gpsReqSentMs > 0 && !gpsTxPending && millis() - gpsReqSentMs > 10000UL) {
        gpsReqSentMs = 0;
        char noGpsBuf[72];
        snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_RESPONSE,BATT:%d",
                 batteryPercent(readVbat()));
        duck.sendData(topics::gps, std::string(noGpsBuf));
    }

    delay(5);
}

// ── handleDuckData ────────────────────────────────────────────────────────────
void handleDuckData(CdpPacket packet) {
    bool isForMe    = (memcmp(packet.dduid.data(), duck.getDuckId().data(), 8) == 0);
    bool isBroadcast = (packet.dduid[0] == 0xFF);

    Serial.printf("[RX] topic=%u duckType=%u isForMe=%d src=%.8s\n",
                  packet.topic, (uint8_t)packet.duckType, (int)isForMe,
                  (char*)packet.sduid.data());

    // ── 1. Extract GPS from packet and update cache ───────────────────────────
    {
        String pdata;
        {
            // Adafruit nRF52 String has no (char*, len) ctor: null-terminate manually.
            std::vector<uint8_t> tmp = packet.data;
            tmp.push_back(0);
            pdata = String((const char*)tmp.data());
        }
        int latIdx = pdata.indexOf("LAT:");
        int lngIdx = pdata.indexOf("LNG:");
        if (latIdx >= 0 && lngIdx >= 0) {
            int latEnd = pdata.indexOf(',', latIdx + 4);
            int lngEnd = pdata.indexOf(',', lngIdx + 4);
            float lat = pdata.substring(latIdx + 4, latEnd < 0 ? (int)pdata.length() : latEnd).toFloat();
            float lng = pdata.substring(lngIdx + 4, lngEnd < 0 ? (int)pdata.length() : lngEnd).toFloat();
            if (!(lat == 0.0f && lng == 0.0f)) {
                char _buf9[9]; memcpy(_buf9, packet.sduid.data(), 8); _buf9[8] = 0;
                String sid(_buf9);
                duckGpsCache[sid] = { lat, lng, millis() };
            }
        }
    }

    // ── 2. Emit CDK:SEEN ─────────────────────────────────────────────────────
    {
        const char* typeStr = "UNKN";
        switch ((uint8_t)packet.duckType) {
            case DuckType::MAMA:     typeStr = "MAMA"; break;
            case DuckType::LINK:     typeStr = "LINK"; break;
            case DuckType::PAPA:     typeStr = "PAPA"; break;
            case DuckType::DETECTOR: typeStr = "DETC"; break;
            default: break;
        }
        char _ssidbuf[9]; memcpy(_ssidbuf, packet.sduid.data(), 8); _ssidbuf[8] = 0;
        String sid(_ssidbuf);
        sid.trim();
        if (sid.length() > 0 && sid != String((const char*)duck.getDuckId().data())) {
            auto gpsIt = duckGpsCache.find(sid);
            if (gpsIt != duckGpsCache.end() && millis() - gpsIt->second.tsMs < DUCK_GPS_TTL_MS) {
                char seenBuf[80];
                snprintf(seenBuf, sizeof(seenBuf), "CDK:SEEN,ID:%.8s,TYPE:%s,LAT:%.6f,LNG:%.6f",
                         (char*)packet.sduid.data(), typeStr,
                         gpsIt->second.lat, gpsIt->second.lng);
                broadcast(seenBuf);
            } else {
                char seenBuf[48];
                snprintf(seenBuf, sizeof(seenBuf), "CDK:SEEN,ID:%.8s,TYPE:%s",
                         (char*)packet.sduid.data(), typeStr);
                broadcast(seenBuf);
            }
        }
    }

    if (!isForMe && !isBroadcast
        && packet.topic != TOPIC_BEACON && packet.topic != TOPIC_BEACON_ACK) return;

    String message;
    {
        std::vector<uint8_t> tmp = packet.data;
        tmp.push_back(0);
        message = String((const char*)tmp.data());
    }
    char replyMsg[200];

    switch (packet.topic) {
        case 22:
            if (message.indexOf("SOS DITERIMA") >= 0) {
                static unsigned long lastSosAckMs = 0;
                if (millis() - lastSosAckMs < 5000UL) break;
                lastSosAckMs         = millis();
                sosAckDisplayPending = true;
                gBeepReq = {1, 500, 0};  // deferred: 1 long beep = SOS acknowledged (relief)
                broadcast("CDK:SOS_ACK,TEXT:SOS DITERIMA");
                break;
            }
            dspPowerSave(0);
            beepBuzzer(1, 150, 0);     // immediate: beep before message appears
            displayMessage(message);
            emergencyDisplayPending = true;
            displayEnabled          = true;
            snprintf(replyMsg, sizeof(replyMsg), "MSG_READ:TEXT:%s", message.c_str());
            duck.sendData(22, replyMsg);
            blinkLed(1);
            broadcast(String("CDK:MSG,TEXT:") + message);
            break;

        case 23:
            beepBuzzer(3, 80, 80);     // immediate: alert before anything else
            flashLED();
            broadcast(String("CDK:MSG,TEXT:") + message);
            duck.sendData(23, "ALERT_ACK");
            break;

        case 24:
            beepBuzzer(3, 80, 80);     // rapid triple = emergency alert (announcement is danger)
            displayAnnouncement(message);
            blinkLed(1);
            broadcast(String("CDK:BCAST,TEXT:") + message);
            break;

        case 25:
            broadcast(String("CDK:PMSG,TEXT:") + message);
            break;

        case 234: {
            // GPS location request
            if (tinyGps.location.isValid()) {
                char gpsBuf[128];
                float altM   = tinyGps.altitude.isValid() ? tinyGps.altitude.meters()  : 0.0f;
                float spdKh  = tinyGps.speed.isValid()    ? tinyGps.speed.kmph()        : 0.0f;
                float hdgDeg = tinyGps.course.isValid()   ? tinyGps.course.deg()        : 0.0f;
                snprintf(gpsBuf, sizeof(gpsBuf),
                         "GPS,LAT:%.6f,LNG:%.6f,ALT:%.1f,SPD:%.1f,HDG:%.1f,SATS:%u,BATT:%d",
                         tinyGps.location.lat(), tinyGps.location.lng(),
                         altM, spdKh, hdgDeg,
                         tinyGps.satellites.value(),
                         batteryPercent(readVbat()));
                dspPowerSave(0);
                dspBegin();
                dspStr(0, 0, ("BATT:" + String(batteryPercent(readVbat())) + "%").c_str());
                dspStrRight(0, idBuf);
                dspStr(0, 14, TXT_SENDING_GPS_DATA);
                dspStr(0, 28, ("LAT:" + String(tinyGps.location.lat(), 5)).c_str());
                dspStr(0, 40, ("LNG:" + String(tinyGps.location.lng(), 5)).c_str());
                dspEnd();
                duck.sendData(topics::gps, std::string(gpsBuf));
                delay(3000);
                dspPowerSave(1);
            } else {
                bool phoneConnected = isPhoneConnected();
                dspPowerSave(0);
                dspBegin();
                dspStr(0, 0, ("BATT:" + String(batteryPercent(readVbat())) + "%").c_str());
                dspStrRight(0, idBuf);
                if (phoneConnected) {
                    if (phoneGpsLatBuf[0] != '\0') {
                        dspStr(0, 14, TXT_SENDING_GPS_DATA);
                        dspStr(0, 28, ("LAT:" + String(phoneGpsLatBuf)).c_str());
                        dspStr(0, 42, ("LNG:" + String(phoneGpsLngBuf)).c_str());
                    } else {
                        dspStrCenter(28, TXT_REQUESTING_GPS_DATA);
                        dspStrCenter(40, TXT_FROM_PHONE_DOTS);
                    }
                    dspEnd();
                    if (gpsReqDeferredSendMs == 0) gpsReqDeferredSendMs = millis() + 400;
                } else {
                    dspStrCenter(28, TXT_NO_PHONE);
                    dspStrCenter(40, TXT_NO_GPS_DATA);
                    dspEnd();
                    char noGpsBuf[64];
                    snprintf(noGpsBuf, sizeof(noGpsBuf), "GPS,FIX:0,SRC:NONE,REASON:NO_PHONE,BATT:%d",
                             batteryPercent(readVbat()));
                    duck.sendData(topics::gps, std::string(noGpsBuf));
                }
                gpsDisplayClearMs = millis() + 2000;
            }
            break;
        }

        case 26:
            if (message.startsWith("[MACK:")) {
                int   end      = message.indexOf(']', 6);
                String ackId   = (end > 6) ? message.substring(6, end) : "";
                String senderId;
                {
                    char buf9[9]; memcpy(buf9, packet.sduid.data(), 8); buf9[8] = 0;
                    senderId = String(buf9);
                }
                broadcast("CDK:MACK,ID:" + ackId + ",FROM:" + senderId);
            } else {
                String text = message;
                String mid  = "";
                int midIdx  = message.lastIndexOf(",MID:");
                if (midIdx >= 0 && (int)(message.length() - midIdx) == 9) {
                    mid  = message.substring(midIdx + 5);
                    text = message.substring(0, midIdx);
                }
                String senderId;
                {
                    char buf9[9]; memcpy(buf9, packet.sduid.data(), 8); buf9[8] = 0;
                    senderId = String(buf9);
                }
                String frameOut = "CDK:MTALK,TEXT:" + text + ",FROM:" + senderId;
                if (mid.length() > 0) frameOut += ",MID:" + mid;
                broadcast(frameOut);
                if (mid.length() > 0) {
                    std::array<uint8_t, 8> senderDuid;
                    for (int i = 0; i < 8; i++) senderDuid[i] = packet.sduid[i];
                    duck.sendData(26, std::string(("[MACK:" + mid + "]").c_str()), senderDuid);
                }
            }
            break;

        case TOPIC_BEACON: {
            if (memcmp(packet.sduid.data(), duck.getDuckId().data(), 8) == 0) break;
            if (!beaconAckPending && !gpsTxPending) {
                char ownGps[80] = {};
                if (tinyGps.location.isValid() && tinyGps.location.age() < 30000) {
                    snprintf(ownGps, sizeof(ownGps), "GPS,LAT:%.6f,LNG:%.6f",
                             tinyGps.location.lat(), tinyGps.location.lng());
                } else if (phoneGpsLatBuf[0] != '\0') {
                    snprintf(ownGps, sizeof(ownGps), "GPS,LAT:%s,LNG:%s",
                             phoneGpsLatBuf, phoneGpsLngBuf);
                } else {
                    strncpy(ownGps, "GPS,FIX:0", sizeof(ownGps) - 1);
                    if (isPhoneConnected() && gpsReqDeferredSendMs == 0)
                        gpsReqDeferredSendMs = millis() + 300;
                }
                strncpy(beaconAckPayload, ownGps, sizeof(beaconAckPayload) - 1);
                beaconAckPending = true;
            }
            break;
        }

        case TOPIC_BEACON_ACK:
            if (memcmp(packet.sduid.data(), duck.getDuckId().data(), 8) == 0) break;
            // GPS extracted in section 1; CDK:SEEN emitted in section 2.
            break;
    }
}

// ── Display helpers ───────────────────────────────────────────────────────────
void displayHome() {
    dspBegin();
    dspStr(0, 0, ("BATT:" + String(batteryPercent(readVbat())) + "%").c_str());
    dspStrRight(0, idBuf);
    // Signal / TX status (no percentage — keeps the line short)
    String sigStr;
    if      (lastSignalPct >= 0 && lastSignalPct <= 25) sigStr = TXT_SIG_WEAK   + String(lastSignalPct) + "%)";
    else if (lastSignalPct >= 0 && lastSignalPct <= 50) sigStr = TXT_SIG_OK   + String(lastSignalPct) + "%)";
    else if (lastSignalPct >= 0 && lastSignalPct <= 75) sigStr = TXT_SIG_STRONG    + String(lastSignalPct) + "%)";
    else if (lastSignalPct >  75)                       sigStr = TXT_SIG_VSTRONG + String(lastSignalPct) + "%)";
    else if (lastTxResult == 0)                         sigStr = TXT_SEND_OK;
    else if (lastTxResult >  0)                         sigStr = TXT_SEND_FAIL;
    else                                                sigStr = TXT_SIG_NONE;
    dspStrCenter(13, sigStr.c_str());
    // GPS status
    char gpsLine[22];
    if (!gpsModuleDetected) {
        strncpy(gpsLine, TXT_GPS_NO_MODULE, sizeof(gpsLine));
    } else if (tinyGps.location.isValid() && tinyGps.location.age() < 5000UL) {
        snprintf(gpsLine, sizeof(gpsLine), "GPS: FIX %uSAT",
                 (unsigned)(tinyGps.satellites.isValid() ? tinyGps.satellites.value() : 0));
    } else {
        snprintf(gpsLine, sizeof(gpsLine), TXT_GPS_SEARCH_FMT,
                 (unsigned)(tinyGps.satellites.isValid() ? tinyGps.satellites.value() : 0));
    }
    dspStrCenter(26, gpsLine);
    dspStrCenter(40, TXT_PRESS_BUTTON_ABOVE);
    dspStrCenter(53, TXT_2SEC_EMERGENCY);
    dspEnd();
}

void displayID() {
    dspStrRight(0, idBuf);
    // Note: caller must wrap with dspBegin()/dspEnd() if needed.
}

void displayBatt() {
    dspStr(0, 0, ("BATT:" + String(batteryPercent(readVbat())) + "%").c_str());
}

void displayMessage(String msg) {
    msg.toUpperCase();
    dspBegin();
    displayID();
    display.setFont(u8g2_font_6x10_tf);
    dspStr(0, 12, msg.substring(0, 21).c_str());
    if (msg.length() > 21) dspStr(0, 24, msg.substring(21, 42).c_str());
    if (msg.length() > 42) dspStr(0, 36, msg.substring(42, 63).c_str());
    dspEnd();
    messagePending  = true;
    displayEnabled  = true;
}

void displayAnnouncement(const String& msg) {
    String upper = msg;
    upper.toUpperCase();
    dspPowerSave(0);
    dspBegin();
    displayID();
    displayBatt();
    dspStrCenter(12, "[EMERGENCY MESSAGE]");
    dspStr(0, 26, upper.substring(0, 21).c_str());
    if (upper.length() > 21) dspStr(0, 38, upper.substring(21, 42).c_str());
    if (upper.length() > 42) dspStr(0, 50, upper.substring(42, 63).c_str());
    dspEnd();
    emergencyDisplayPending = true;
    displayEnabled          = true;
}

// ── LED ───────────────────────────────────────────────────────────────────────
void flashLED() {
    for (int i = 0; i < 5; i++) {
        digitalWrite(PIN_LED1, HIGH);
        delay(200);
        digitalWrite(PIN_LED1, LOW);
        delay(200);
    }
}

void blinkLed(int times) {
    for (int n = 0; n < times; n++) {
        digitalWrite(PIN_LED1, HIGH);
        { unsigned long t = millis(); while (millis() - t < 200) { duck.run(); delay(5); } }
        digitalWrite(PIN_LED1, LOW);
        { unsigned long t = millis(); while (millis() - t < 200) { duck.run(); delay(5); } }
    }
}

// Buzzer is D12 = P1.00 (passive buzzer — needs PWM at resonant frequency to be loud).
// Safe to call from duck.run() callbacks: off-gaps use delay() not duck.run(),
// avoiding recursive re-entry into the CDP stack.
void beepBuzzer(int times, int onMs, int offMs) {
    NRF_P1->DIRSET = (1u << 0);   // ensure D12 is output
    for (int n = 0; n < times; n++) {
        // Bit-bang ~2.5 kHz square wave for the on duration.
        unsigned long endMs = millis() + (unsigned long)onMs;
        while ((long)(endMs - millis()) > 0) {
            NRF_P1->OUTSET = (1u << 0);
            delayMicroseconds(200);
            NRF_P1->OUTCLR = (1u << 0);
            delayMicroseconds(200);
        }
        // Off gap: simple delay — safe from any call context.
        if (n < times - 1 && offMs > 0) {
            delay(offMs);
        }
    }
    NRF_P1->OUTCLR = (1u << 0);   // ensure buzzer is silent after last beep
}

// ── Battery ───────────────────────────────────────────────────────────────────
void sendBattery() {
    int pct = batteryPercent(readVbat());
    broadcast("CDK:BATT,LEVEL:" + String(pct));
}

// ── SOS ───────────────────────────────────────────────────────────────────────
bool sendEmergency(String lat, String lng, String alt, String spd, String hdg, bool gpsFromPhone) {
    bool hasGps  = (lat.length() > 0 && lng.length() > 0);
    int  battPct = batteryPercent(readVbat());

    std::string loraMsg = "SOS,SRC:DEVICE,ID:" + std::string(DUCK_ID_BUF);
    if (hasGps) {
        loraMsg += ",LAT:" + std::string(lat.c_str()) + ",LNG:" + std::string(lng.c_str());
        if (alt.length() > 0) loraMsg += ",ALT:" + std::string(alt.c_str());
        if (spd.length() > 0) loraMsg += ",SPD:" + std::string(spd.c_str());
        if (hdg.length() > 0) loraMsg += ",HDG:" + std::string(hdg.c_str());
        if (gpsFromPhone)      loraMsg += ",GPS:PHONE";
    }
    loraMsg += ",BATT:" + std::to_string(battPct);

    int failure = duck.sendData(topics::alert, loraMsg);
    lastTxResult = failure;
    lastTxMs     = millis();

    if (!failure) {
        counter++;
        dspPowerSave(0);
        blinkLed(3);
        String sosFrame = String("CDK:SOS,SRC:DEVICE,ID:") + DUCK_ID_BUF +
                                 ",LAT:" + (hasGps ? lat : "none") +
                          ",LNG:" + (hasGps ? lng : "none");
        if (hasGps && alt.length() > 0) sosFrame += ",ALT:" + alt;
        if (hasGps && spd.length() > 0) sosFrame += ",SPD:" + spd;
        if (hasGps && hdg.length() > 0) sosFrame += ",HDG:" + hdg;
        if (hasGps && gpsFromPhone)      sosFrame += ",GPS:PHONE";
        sosFrame += ",BATT:" + String(battPct);
        broadcast(sosFrame);
        dspBegin();
        dspStrRight(0, idBuf);
        displayBatt();
        dspStrCenter(22, TXT_SEND_OK);
        dspStrCenter(34, TXT_EMERGENCY_SIGNAL);
        dspStrCenter(46, hasGps ? TXT_WITH_GPS : TXT_WITHOUT_GPS);
        dspEnd();
        blinkLed(2);
        if (!hasGps) {
            beepBuzzer(2, 60, 60);  // distinct warning: sent, but no location included
        }
        {
            unsigned long endMs = millis() + 2000UL;
            while (millis() < endMs) { duck.run(); delay(10); }
        }
        displayHome();
    } else {
        dspBegin();
        dspStrRight(0, idBuf);
        displayBatt();
        dspStrCenter(22, TXT_SOS_ERR_CANNOT);
        dspStrCenter(34, TXT_SEND_SIGNAL);
        dspStrCenter(46, TXT_EMERGENCY);
        dspEnd();
        beepBuzzer(2, 60, 60);      // SOS failed — fast double = error
    }
    return true;
}

// ── Phone connection detection ────────────────────────────────────────────────
static bool isPhoneConnected() {
    return usbPhoneSeen || blePhoneSeen;
}

// ── Broadcast on all active channels ─────────────────────────────────────────
void broadcast(const String& frame) {
    String payload = frame.endsWith("\n") ? frame : frame + "\n";
    Serial.print(payload);
    bleSendLine(payload);
}

// ── Frame dispatcher ──────────────────────────────────────────────────────────
void handleFrame(const String& line) {
    if (!line.startsWith("CDK:")) return;
    broadcast(String("CDK:ID,VALUE:") + DUCK_ID_BUF);
    String body   = line.substring(4);
    int    comma  = body.indexOf(',');
    String type   = (comma == -1) ? body : body.substring(0, comma);

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
        case FT_SOS:   handleSOS(body);   break;
        case FT_MSG:   handleMsg(body);   break;
        case FT_PING:  /* ID already broadcast */ break;
        case FT_MTALK: handleMamaTalk(body); break;
        case FT_GPS:   handleGps(body);   break;
        case FT_SCAN: {
            char gpsPayload[80] = {};
            if (tinyGps.location.isValid() && tinyGps.location.age() < 30000) {
                snprintf(gpsPayload, sizeof(gpsPayload), "GPS,LAT:%.6f,LNG:%.6f",
                         tinyGps.location.lat(), tinyGps.location.lng());
            } else if (phoneGpsLatBuf[0] != '\0') {
                snprintf(gpsPayload, sizeof(gpsPayload), "GPS,LAT:%s,LNG:%s",
                         phoneGpsLatBuf, phoneGpsLngBuf);
            } else {
                strncpy(gpsPayload, "GPS,FIX:0", sizeof(gpsPayload) - 1);
                if (isPhoneConnected() && gpsReqDeferredSendMs == 0 && gpsReqSentMs == 0)
                    gpsReqDeferredSendMs = millis() + 300;
            }
            int beaconResult = duck.sendData(TOPIC_BEACON, std::string(gpsPayload), BROADCAST_DUID);
            broadcast(beaconResult == 0 ? "CDK:STATUS,SCAN:ping_sent" : "CDK:STATUS,SCAN:ping_failed");
            broadcast("CDK:SCAN_ACK");
            break;
        }
        case FT_BYE:
            usbPhoneSeen = false; usbDisconnectDisplayPending = true;
            break;
        default: break;
    }
}

// ── SOS from phone ────────────────────────────────────────────────────────────
void handleSOS(const String& body) {
    String lat = extractField(body, "LAT");
    String lng = extractField(body, "LNG");
    String alt = extractField(body, "ALT");
    String spd = extractField(body, "SPD");
    String hdg = extractField(body, "HDG");
    int battPct = batteryPercent(readVbat());

    dspPowerSave(0);
    dspBegin();
    dspStrRight(0, idBuf);
    displayBatt();
    dspStrCenter(22, TXT_SENDING);
    dspStrCenter(34, TXT_EMERGENCY_SIGNAL_DOTS);
    dspEnd();

    String message = "SOS,LAT:" + lat + ",LNG:" + lng;
    if (alt.length() > 0) message += ",ALT:" + alt;
    if (spd.length() > 0) message += ",SPD:" + spd;
    if (hdg.length() > 0) message += ",HDG:" + hdg;
    message += ",BATT:" + String(battPct);

    int failure = duck.sendData(topics::status, std::string(message.c_str()));
    blinkLed(3);
    broadcast("CDK:ACK,ID:SOS");

    dspPowerSave(0);
    dspBegin();
    dspStrRight(0, idBuf);
    displayBatt();
    dspStrCenter(22, TXT_SEND_OK);
    dspStrCenter(34, TXT_EMERGENCY_SIGNAL);
    dspStrCenter(46, TXT_WITH_GPS);
    dspEnd();
    emergencyDisplayPending = true;
    displayEnabled          = true;
}

// ── Message from phone ────────────────────────────────────────────────────────
void handleMsg(const String& body) {
    String urgency = extractField(body, "URGENCY");
    String lat     = extractField(body, "LAT");
    String lng     = extractField(body, "LNG");
    String text    = extractField(body, "TEXT");
    String message = "MSG,URGENCY:" + urgency + ",LAT:" + lat + ",LNG:" + lng + ",TEXT:" + text;

    dspPowerSave(0);
    dspBegin();
    dspStrRight(0, idBuf);
    displayBatt();
    dspStrCenter(22, TXT_MSG_SENT);
    dspEnd();
    blinkLed(1);
    displayHome();

    int failure = duck.sendData(topics::status, std::string(message.c_str()));
    if (!failure) broadcast("CDK:ACK,ID:MSG");
}

// ── MamaDuck-to-MamaDuck talk ─────────────────────────────────────────────────
bool sendMamaTalk(const String& targetId, const String& msg, const String& mid) {
    if (targetId.length() != 8) return false;
    std::array<uint8_t, 8> targetDuid =
        duckutils::stringToArray<uint8_t, 8>(std::string(targetId.c_str()));
    String payload = msg;
    if (mid.length() > 0) payload += ",MID:" + mid;
    int failure = duck.sendData(26, std::string(payload.c_str()), targetDuid);
    if (!failure) broadcast("CDK:ACK,ID:MTALK,TARGET:" + targetId);
    return !failure;
}

void handleMamaTalk(const String& body) {
    String target = extractField(body, "TARGET");
    String text   = extractField(body, "TEXT");
    String mid    = extractField(body, "MID");
    if (target.length() == 0) return;
    sendMamaTalk(target, text, mid);
}

// ── GPS from phone ────────────────────────────────────────────────────────────
void handleGps(const String& body) {
    gpsReqSentMs = 0;
    String lat = extractField(body, "LAT");
    String lng = extractField(body, "LNG");
    if (lat.length() == 0 || lat == "none" || lng.length() == 0 || lng == "none") {
        phoneGpsNoFix          = true;
        phoneGpsDisplayPending = true;
        snprintf(gpsTxPayload, sizeof(gpsTxPayload),
                 "GPS,FIX:0,SRC:PHONE,REASON:NO_SIGNAL,BATT:%d",
                 batteryPercent(readVbat()));
        gpsTxPending = true;
        return;
    }
    char gpsBuf[128];
    snprintf(gpsBuf, sizeof(gpsBuf), "GPS,SRC:PHONE,LAT:%s,LNG:%s", lat.c_str(), lng.c_str());
    strncpy(phoneGpsLatBuf, lat.c_str(), sizeof(phoneGpsLatBuf) - 1);
    strncpy(phoneGpsLngBuf, lng.c_str(), sizeof(phoneGpsLngBuf) - 1);
    String alt = extractField(body, "ALT");
    String spd = extractField(body, "SPD");
    String hdg = extractField(body, "HDG");
    phoneGpsAltBuf[0] = '\0';
    phoneGpsSpdBuf[0] = '\0';
    phoneGpsHdgBuf[0] = '\0';
    if (alt.length() > 0) { strncpy(phoneGpsAltBuf, alt.c_str(), sizeof(phoneGpsAltBuf) - 1); strncat(gpsBuf, (",ALT:" + alt).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1); }
    if (spd.length() > 0) { strncpy(phoneGpsSpdBuf, spd.c_str(), sizeof(phoneGpsSpdBuf) - 1); strncat(gpsBuf, (",SPD:" + spd).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1); }
    if (hdg.length() > 0) { strncpy(phoneGpsHdgBuf, hdg.c_str(), sizeof(phoneGpsHdgBuf) - 1); strncat(gpsBuf, (",HDG:" + hdg).c_str(), sizeof(gpsBuf) - strlen(gpsBuf) - 1); }
    char battSuffix[16];
    snprintf(battSuffix, sizeof(battSuffix), ",BATT:%d", batteryPercent(readVbat()));
    strncat(gpsBuf, battSuffix, sizeof(gpsBuf) - strlen(gpsBuf) - 1);
    phoneGpsNoFix          = false;
    phoneGpsDisplayPending = true;
    strncpy(gpsTxPayload, gpsBuf, sizeof(gpsTxPayload) - 1);
    gpsTxPending = true;
}

// ── Utilities ─────────────────────────────────────────────────────────────────
String extractField(const String& body, const String& key) {
    String search = key + ":";
    int    idx    = body.indexOf(search);
    if (idx == -1) return "";
    int start = idx + search.length();
    int end   = body.indexOf(',', start);
    return (end == -1) ? body.substring(start) : body.substring(start, end);
}
