/**
 * @file MamaDuck.ino — Seeed Wio Tracker L1 Pro (nRF52840 + SX1262)
 * @brief MamaDuck example using ClusterDuck Protocol on the nRF52840 platform.
 *
 * Hardware: Seeed Wio Tracker L1 Pro
 *   - MCU:     nRF52840 @ 64 MHz
 *   - Radio:   SX1262 (SPI, TCXO 1.8 V, DIO2 RF switch)
 *   - Display: SH1106 128×64 OLED (I2C)
 *   - GPS:     L76KB NMEA (Serial1, 9600 baud)
 *   - BLE:     Nordic UART Service (NUS) via raw SoftDevice S140 v7 API.
 *              The Adafruit Bluefruit wrapper is NOT used — it calls
 *              nrfx_power_uninit() via usb_softdevice_pre_enable(), which
 *              races with usb_device_task (FreeRTOS) and kills USB CDC.
 *              Raw SD API avoids that wrapper while replicating only the
 *              pre/post USB-handover steps required for coexistence.
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
#include <map>
#include <CDP.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <TinyGPSPlus.h>

// ── BLE enable flag ───────────────────────────────────────────────────────────
// Set to 1 to enable Bluetooth LE (Nordic UART Service via Bluefruit52Lib).
// Requires SVC dispatcher + SoftDevice S140 infrastructure in setup().
#define ENABLE_BLE 0

#if ENABLE_BLE
#include <bluefruit.h>   // Bluefruit52Lib: Bluefruit singleton + BLEUart
#endif
#include "image.h"

// Access the RadioLib radio instance from DuckLoRa.cpp to read RSSI/SNR.
extern CDPCFG_LORA_CLASS lora;

// ── Board sanity check ────────────────────────────────────────────────────────
#ifndef ARDUINO_SEEED_WIO_TRACKER_L1
#error "This sketch is for the Seeed Wio Tracker L1 Pro. \
Define ARDUINO_SEEED_WIO_TRACKER_L1 (or use env:local_wio_tracker_l1)."
#endif

// ── Identification ────────────────────────────────────────────────────────────
#define DUCK_NAME "IBRAHIM1"   // MUST be exactly 8 bytes and unique on the mesh

// ── GPS ───────────────────────────────────────────────────────────────────────
static TinyGPSPlus tinyGps;
static bool gpsModuleDetected = false;
static bool gpsFix            = false;

// ── Display ───────────────────────────────────────────────────────────────────
// SH1106 128×64 hardware I2C at address 0x3D (confirmed by I2C scan).
// U8G2 uses 8-bit addresses internally (7-bit << 1), so setI2CAddress(0x7A)
// is called before every display.begin().  Wire.begin() configures NRF_TWIM0
// directly; see setupBLE() for the TWIM0 release/reinit around sd_enable.
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// U8g2 uses baseline y-coordinates.  These helpers mirror the heltec library's
// top-left convention so ported code can use integer pixel rows (0 = top).
// All measurements are for u8g2_font_6x10_tf:  ascent = 8 px, height = 10 px.
#define DSP_LINE_H  13   // pixel rows between lines (generous for readability)
#define DSP_ASCENT   8   // ascent of u8g2_font_6x10_tf

// Declare here so Arduino's auto-prototype generator sees it before any function
// that returns this type (enum must precede its first use in generated .cpp).
enum BtnEvent { BTN_NONE, BTN_SINGLE, BTN_TRIPLE, BTN_QUAD, BTN_HOLD_2S };

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

// Probe and initialise the SSD1306 OLED early in setup().
static void initDisplay() {
    // Give the display time to power up before first I2C access.
    delay(50);
    Wire.begin();

    // Auto-detect I2C address — try 0x3D first, fall back to 0x3C.
    // SA0 pin on SH1106 sets address: HIGH=0x3D, LOW=0x3C.
    uint8_t foundAddr = 0;
    for (uint8_t a : {(uint8_t)0x3D, (uint8_t)0x3C}) {
        Wire.beginTransmission(a);
        Wire.write(0x00);   // avoids TWIM_109 errata (MAXCNT=0 corrupts TX)
        if (Wire.endTransmission() == 0) { foundAddr = a; break; }
    }

    if (foundAddr == 0) {
        // No display found — 4 fast blinks so the user can see it without serial.
        for (int i = 0; i < 4; i++) {
            NRF_P1->OUTSET = (1u<<1); delay(80);
            NRF_P1->OUTCLR = (1u<<1); delay(80);
        }
        Serial.println("[DISP] ERROR: no SH1106 found at 0x3D or 0x3C"); Serial.flush();
        return;   // gDisplayOk stays false
    }
    Serial.printf("[DISP] SH1106 found at 0x%02X\n", foundAddr); Serial.flush();

    display.setI2CAddress(foundAddr << 1);
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
    Serial.printf("[DISP] init OK (addr=0x%02X)\n", foundAddr); Serial.flush();
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

#if ENABLE_BLE
// ── SVC Dispatcher: SoftDevice SVC forwarding ────────────────────────────────
// The meshcore-dev BSP fork's FreeRTOSConfig.h defines:
//   #define vPortSVCHandler SVC_Handler
// This makes port.c compile the naked FreeRTOS task-start handler as the ONLY
// SVC_Handler.  SoftDevice API calls (sd_softdevice_enable etc.) use SVC 0x10+;
// hitting the FreeRTOS handler with those numbers corrupts CPU state → HardFault.
//
// Fix (no BSP modification): relocate the vector table to RAM, replace the SVC
// slot (entry 11) with svc_dispatch, and update SCB->VTOR.
//   SVC 0   → original FreeRTOS handler (task-start, called once at scheduler init)
//   SVC ≥ 1 → MBR SVC_Handler at *(uint32_t*)0x0000002C (nRF52840 always-present)
//             The MBR routes the call to S140 before / at SD enable time.
//             After sd_softdevice_enable(), the SD installs itself in VTOR and
//             intercepts all SD SVCs directly — svc_dispatch is no longer used.

#define VTOR_NUM_VECTORS  80U   // nRF52840: 16 Cortex-M4 core + 64 peripheral IRQs
static uint32_t g_ramVectors[VTOR_NUM_VECTORS] __attribute__((aligned(512)));
// C linkage so the inline assembly pseudo-instruction "ldr r0, =name" can find it.
extern "C" uint32_t g_freeRtosSvcHandler;
uint32_t g_freeRtosSvcHandler;
// Forward declaration — defined later in this file, installed into g_ramVectors[3].
extern "C" void HardFault_Handler(void);

static __attribute__((naked)) void svc_dispatch(void) {
    __asm volatile (
        // Determine caller stack: PSP (thread mode) or MSP (handler mode).
        "tst    lr, #4          \n"
        "ite    eq              \n"
        "mrseq  r0, msp         \n"
        "mrsne  r0, psp         \n"
        // Stacked PC is at frame offset +24.
        // The SVC instruction that caused the exception is 2 bytes before PC.
        "ldr    r1, [r0, #24]   \n"
        "ldrb   r2, [r1, #-2]   \n"   // SVC immediate (0-255)
        // SVC 0 = FreeRTOS (first-task start) — tail-call the saved handler.
        "cmp    r2, #0          \n"
        "bne    1f              \n"
        "ldr    r0, =g_freeRtosSvcHandler \n"
        "ldr    r0, [r0]        \n"
        "bx     r0              \n"
        // SVC ≥ 1 = SoftDevice — forward DIRECTLY to S140's own SVC_Handler.
        // S140 vector table base is 0x1000; SVC_Handler is entry 11 → offset 0x2C.
        // So S140's SVC_Handler address lives at 0x102C.
        //
        // NOTE: We do NOT route through the MBR's SVC_Handler (at *(0x002C)).
        // The MBR's handler for SVC != 0x18 reads the *registered* app-vector-table
        // base (0x27000, set by the bootloader) and jumps to FLASH[0x27000+0x2C] =
        // vPortSVCHandler in the flash table — completely wrong for SD SVCs.
        // Going directly to S140's SVC_Handler bypasses that broken path.
        "1:                     \n"
        "movs   r0, #0x10       \n"   // r0 = 0x10
        "lsls   r0, r0, #8      \n"   // r0 = 0x1000 (S140 vector table base)
        "ldr    r0, [r0, #0x2C] \n"   // r0 = *(0x102C) = S140's SVC_Handler
        "bx     r0              \n"
    );
}

// Call once from setup() before any sd_* API call.
static void install_svc_dispatcher(void) {
    const uint32_t* vt = (const uint32_t*)SCB->VTOR;
    for (uint32_t i = 0; i < VTOR_NUM_VECTORS; ++i)
        g_ramVectors[i] = vt[i];
    // Save the FreeRTOS SVC_Handler (entry 11) and replace with our dispatcher.
    g_freeRtosSvcHandler = g_ramVectors[11];
    g_ramVectors[11] = ((uint32_t)svc_dispatch) | 1u;   // Thumb bit set
    // Also install our custom HardFault handler (entry 3) for diagnostics.
    g_ramVectors[3]  = ((uint32_t)HardFault_Handler) | 1u;
    // Commit: memory barrier then update VTOR.
    __DMB();
    SCB->VTOR = (uint32_t)g_ramVectors;
    __DSB();
    __ISB();
    // Diagnostic: read S140 SVC_Handler address from 0x102C.
    // If ACL-protected, this causes a HardFault (printed by our handler above).
    uint32_t s140svc = *(volatile const uint32_t*)0x102CUL;
    Serial.printf("[SVC] g_ramVectors=0x%08X VTOR=0x%08X 0x102C=0x%08X\n",
                  (unsigned)g_ramVectors, (unsigned)SCB->VTOR, (unsigned)s140svc);
    Serial.println("[SVC] dispatcher installed (SD SVC forwarding active)");
    Serial.flush();
}
#endif // ENABLE_BLE

#if ENABLE_BLE
// ── HardFault handler ───────────────────────────────────────────────────────
// Print stacked PC, LR, CFSR on HardFault then reset. Installed via g_ramVectors
// noinit section survives soft reset — used to pass fault info to next boot.
#define FAULT_MAGIC 0xBEEFDEADU
__attribute__((section(".noinit"))) static uint32_t gFaultMagic;
__attribute__((section(".noinit"))) static uint32_t gFaultPC;
__attribute__((section(".noinit"))) static uint32_t gFaultLR;
__attribute__((section(".noinit"))) static uint32_t gFaultCFSR;
__attribute__((section(".noinit"))) static uint32_t gFaultHFSR;
// BLE init step tracker — survives reset so we can print it on next boot.
// Written before each sd_* call; read on next boot via printBleStep().
#define BLE_STEP_MAGIC 0xBEEC0FFEU
__attribute__((section(".noinit"))) static uint32_t gBleStepMagic;
__attribute__((section(".noinit"))) static uint32_t gBleStep;  // last reached step
__attribute__((section(".noinit"))) static uint32_t gBleRc;    // last error code

// (VTOR relocation) so it overrides the weak BSP stub.
// IMPORTANT: This runs in exception context — NO delay(), NO vTaskDelay(), NO Serial.
// delay() calls vTaskDelay() which is illegal from an ISR and triggers configASSERT,
// hanging the device silently. We only write to noinit RAM then reset; the
// [FAULT-PREV] block at the top of setup() prints everything on the next boot.
static void __attribute__((noinline)) hardFaultPrint(uint32_t* frame, uint32_t lr) {
    gFaultPC    = frame[6];
    gFaultLR    = frame[5];
    gFaultCFSR  = SCB->CFSR;
    gFaultHFSR  = SCB->HFSR;
    gFaultMagic = FAULT_MAGIC;
    // Blink 5 fast times using raw registers (safe in exception context)
    NRF_P1->DIRSET = (1u<<1);
    for (int i = 0; i < 5; i++) {
        NRF_P1->OUTSET = (1u<<1); for(volatile uint32_t d=0; d<3200000u; d++) {}
        NRF_P1->OUTCLR = (1u<<1); for(volatile uint32_t d=0; d<3200000u; d++) {}
    }
    NVIC_SystemReset();
}
extern "C" void __attribute__((naked)) HardFault_Handler(void) {
    __asm volatile(
        "tst    lr, #4   \n"
        "ite    eq       \n"
        "mrseq  r0, msp  \n"
        "mrsne  r0, psp  \n"
        "mov    r1, lr   \n"
        "b      %0       \n"
        : : "i"(hardFaultPrint) : "r0", "r1"
    );
}
#endif // ENABLE_BLE

// ── BLE state ────────────────────────────────────────────────────────────────
#if ENABLE_BLE
static BLEUart                bleuart;
static uint16_t               bleConnHandle    = BLE_CONN_HANDLE_INVALID;
static bool                   blePhoneSeen     = false;
static unsigned long          lastBleRxMs      = 0;
static char                   bleRxLine[256]   = {};
static volatile bool          bleRxPending     = false;
static String                 bleInBuf         = "";
#else
// BLE disabled — minimal stubs so non-BLE code compiles unchanged.
static bool                   blePhoneSeen     = false;
static unsigned long          lastBleRxMs      = 0;
static char                   bleRxLine[256]   = {};
static volatile bool          bleRxPending     = false;
#endif

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
static bool          messagePending       = false;
static unsigned long messagePendingMs     = 0;
const  unsigned long MESSAGE_DISPLAY_MS   = 10000UL;
static bool          emergencyDisplayPending = false;

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
bool sendEmergency(String lat = "", String lng = "", String alt = "",
                   String spd = "", String hdg = "", bool gpsFromPhone = false);
static float readVbat();
static int batteryPercent(float vbat);
static bool isPhoneConnected();
#if ENABLE_BLE
static void setupBLE();
static void bleSendLine(const String& line);
#else
static inline void bleSendLine(const String&) {}  // no-op when BLE disabled
#endif

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

#if ENABLE_BLE
// ── Bluefruit BLE implementation ─────────────────────────────────────────────
//
// Bluefruit.begin() handles the complete SD + BLE enable sequence:
//   usb_softdevice_pre_enable()  → nrfx_power teardown (same 3 calls as before)
//   sd_softdevice_enable()       → S140 v7, RC clock (USE_LFRC from variant.h)
//   usb_softdevice_post_enable() → re-register USB power events via SD
//   sd_ble_cfg_set() + sd_ble_enable()
// Bluefruit's internal adafruit_soc_task FreeRTOS task handles SD SOC events
// (USB power reconnect) and BLE events, replacing our manual ble_soc_task.

// Connect / disconnect callbacks — called from Bluefruit's event task context.
static void bleConnectCb(uint16_t conn_handle) {
    bleConnHandle = conn_handle;
    blePhoneSeen  = true;
    dspStatus("Connected", DUCK_NAME);
    Serial.printf("[BLE] Phone connected (h=%u)\n", conn_handle); Serial.flush();
}
static void bleDisconnectCb(uint16_t conn_handle, uint8_t reason) {
    (void)conn_handle;
    bleConnHandle = BLE_CONN_HANDLE_INVALID;
    blePhoneSeen  = false;
    dspStatus("Advertising", DUCK_NAME);
    Serial.printf("[BLE] Disconnected (reason=0x%02X)\n", reason); Serial.flush();
}

// BLE UART RX callback — called from Bluefruit event task when data arrives.
static void bleUartRxCb(uint16_t /*conn_handle*/) {
    while (bleuart.available()) {
        char c = (char)bleuart.read();
        if (c == '\n') {
            if (!bleRxPending) {
                strncpy(bleRxLine, bleInBuf.c_str(), sizeof(bleRxLine)-1);
                bleRxLine[sizeof(bleRxLine)-1] = '\0';
                bleRxPending = true;
            }
            bleInBuf = "";
        } else if (c != '\r') {
            bleInBuf += c;
        }
    }
    lastBleRxMs = millis();
}

// Send a text line to the connected phone via NUS TX notifications.
static void bleSendLine(const String& line) {
    if (!bleuart.notifyEnabled()) return;
    String payload = line.endsWith("\n") ? line : line + "\n";
    bleuart.write((const uint8_t*)payload.c_str(),
                  (size_t)min((int)payload.length(), 512));
}

// Enable BLE stack, register NUS, start advertising.
static void setupBLE() {
    Serial.println("[BLE] setupBLE() entered"); Serial.flush();
    if (*(volatile const uint32_t*)0x00001000U == 0xFFFFFFFFU) {
        Serial.println("[BLE] S140 not in flash — BLE disabled"); return;
    }
    Serial.println("[BLE] S140 found"); Serial.flush();
    dspStatus("BLE init", nullptr);

    gBleStepMagic = BLE_STEP_MAGIC;
    gBleStep = 0; gBleRc = 0;

    // Bluefruit.begin(peripheral, central) calls usb_softdevice_pre_enable() then
    // sd_softdevice_enable() then usb_softdevice_post_enable() then sd_ble_enable().
    // This is the BSP-native path — no manual TWIM0/PSEL teardown needed or wanted.
    //
    // ── Priority-3 IRQ quarantine ─────────────────────────────────────────────
    // S140 sd_softdevice_enable() returns NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION
    // if ANY application IRQ is enabled at priority 0–4 when it is called.
    // Three BSP drivers set their IRQs to priority 3 and may be enabled:
    //   • Wire_nRF52.cpp:  SPIM0_SPIS0_TWIM0 at priority 3 — set by initDisplay()
    //   • Uart.cpp:        UARTE0_UART0      at priority 3 — set by Serial1.begin()
    //   • tusb_hal_nrf.c:  USBD_IRQn         at priority 3 — set when USB cable is
    //                      connected; usb_softdevice_pre_enable() (called inside
    // Quarantine only the two APP-priority IRQs (priority 3) that conflict with
    // sd_softdevice_enable().  USBD and POWER_CLOCK must NOT be disabled here:
    //   POWER_CLOCK — the SD and usb_softdevice_pre_enable(→nrfx_power_uninit) manage
    //                 this IRQ; disabling it prevents the SD from detecting USB VBUS.
    //   USBD       — TinyUSB manages this; usb_softdevice_post_enable re-enables it
    //                 via the SD event task (tusb_hal_nrf_power_event).  Disabling it
    //                 here breaks the USB CDC re-enumeration after SD enable.
    Serial.println("[BLE] disabling priority-3 IRQs (TWIM0 + UARTE0 only)..."); Serial.flush();
    NVIC_DisableIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);  // Wire/TWIM0 at prio 3
    NVIC_ClearPendingIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
    NVIC_DisableIRQ(UARTE0_UART0_IRQn);                         // Serial/UARTE0 at prio 3
    NVIC_ClearPendingIRQ(UARTE0_UART0_IRQn);
    // NOTE: do NOT call vTaskSuspendAll() here.  Bluefruit.begin() creates
    // FreeRTOS semaphores and tasks internally, and pvPortMalloc() suspends
    // the scheduler itself for heap operations — nesting causes issues.

    // Diagnostic: dump NVIC ISER so we can see which IRQs are active.
    Serial.printf("[BLE] NVIC ISER[0]=0x%08lX ISER[1]=0x%08lX\n",
                  (unsigned long)NVIC->ISER[0], (unsigned long)NVIC->ISER[1]);
    Serial.flush();

    gBleStep = 1;
    // Do NOT write GPREGRET before Bluefruit.begin() — if a crash occurs before the
    // success path clears it, the Adafruit bootloader may misinterpret the value.
    Serial.println("[BLE] calling Bluefruit.begin..."); Serial.flush();
    bool _bleOk = Bluefruit.begin(1, 0);
    if (!_bleOk) {
        gBleStep = 0x10; gBleRc = 1;
        // SD never initialized — restore quarantined IRQs (TWIM0 + UARTE0 only).
        NVIC_EnableIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
        NVIC_EnableIRQ(UARTE0_UART0_IRQn);
        Serial.println("[BLE] Bluefruit.begin FAILED"); Serial.flush();
        dspStatus("BLE FAIL", "begin()");
        BLINK_LED(8);
        return;
    }
    // After sd_softdevice_enable() the SD owns NRF_POWER — direct register
    // writes to GPREGRET etc. cause a HardFault.  Use sd_power_gpregret_set()
    // if a post-SD GPREGRET write is ever needed.
    // Re-enable the two quarantined app-priority IRQs.
    // USBD + POWER_CLOCK are handled by the SD event task (usb_softdevice_post_enable).
    NVIC_EnableIRQ(UARTE0_UART0_IRQn);                           // GPS Serial1
    NVIC_EnableIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);     // Wire/TWIM0
    BLINK_LED(2); // 2 blinks: SD + BLE stack on

    // Display was already initialised in initDisplay(); Wire/TWIM0 is unaffected by
    // Bluefruit.begin() — no re-init needed.  Just update the status text.
    dspStatus("BLE: SD on", nullptr);

    gBleStep = 5;
    Bluefruit.setName(DUCK_NAME);
    Bluefruit.Periph.setConnectCallback(bleConnectCb);
    Bluefruit.Periph.setDisconnectCallback(bleDisconnectCb);

    // NUS service (Nordic UART Service, full 128-bit UUID) via BLEUart.
    gBleStep = 6;
    bleuart.setRxCallback(bleUartRxCb);
    bleuart.begin();
    dspStatus("BLE: stack on", nullptr);
    BLINK_LED(4); // 4 blinks: NUS service registered

    // Advertising: name in adv packet so nRF Connect sees it during passive scan.
    // Service UUID (128-bit, 18 bytes) goes to scan response to stay under 31-byte limit.
    // Packet breakdown: Flags(3) + TxPower(3) + Name(10) = 16 bytes ✓
    gBleStep = 7;
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addName();            // device name "IBRAHIM1" in adv packet
    Bluefruit.ScanResponse.addService(bleuart); // NUS 128-bit UUID in scan response
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244); // 20 ms fast / 152.5 ms slow
    Bluefruit.Advertising.setFastTimeout(30);   // fast mode for 30 s
    bool _advOk = Bluefruit.Advertising.start(0);
    if (!_advOk) {
        gBleStep = 0x20;
        Serial.println("[BLE] Advertising.start() returned FALSE"); Serial.flush();
        dspStatus("ADV FAIL", "start() false");
        // 3 groups of 2 blinks = advertising start failed
        BLINK_LED(2); BLINK_LED(2); BLINK_LED(2);
        return;  // gBleStep != 99 → setup() will catch this
    }

    dspStatus("Advertising", DUCK_NAME);
    BLINK_LED(6); // 6 blinks: advertising CONFIRMED running

    gBleStep = 99;
    // SD owns NRF_POWER after sd_softdevice_enable — do not write GPREGRET directly.
    Serial.printf("[BLE] Advertising as '%s' (NUS/BLEUart)\n", DUCK_NAME); Serial.flush();
}
#endif // ENABLE_BLE

// ── Battery ADC ───────────────────────────────────────────────────────────────
static float readVbat() {
    // Drive BAT_CTL HIGH to enable the battery voltage-divider (active HIGH on
    // Seeed nRF52840 designs — a LOW gate keeps the switch open).
    pinMode(BAT_CTL, OUTPUT);
    digitalWrite(BAT_CTL, HIGH);
    delay(5);
    // analogReadResolution(ADC_RESOLUTION) is called in setup(), so
    // analogRead() returns a 14-bit value (0–16383).
    float raw  = (float)analogRead(PIN_VBAT_READ);
    float vbat = raw / (float)((1 << ADC_RESOLUTION) - 1) * AREF_VOLTAGE * ADC_MULTIPLIER;
    // Return BAT_CTL pin to input (Hi-Z) to save power.
    pinMode(BAT_CTL, INPUT);
    return vbat;
}

static int batteryPercent(float vbat) {
    // LiPo: 4.2 V = 100%, 3.5 V = 0%.  Adjust thresholds as needed.
    float pct = (vbat - 3.5f) / (4.2f - 3.5f) * 100.0f;
    return (int)constrain(pct, 0.0f, 100.0f);
}

// ── Button debouncer ──────────────────────────────────────────────────────────
// (enum BtnEvent declared earlier near top of file)

static BtnEvent checkButton() {
    static bool     wasDown      = false;
    static uint32_t pressStartMs = 0;
    static uint8_t  clickCount   = 0;
    static uint32_t lastReleaseMs = 0;
    static bool     holdFired    = false;

    const uint32_t HOLD_MS   = 2000;
    const uint32_t CLICK_GAP = 400;   // max ms between clicks in a multi-click burst

    bool btnDown = (digitalRead(PIN_BUTTON1) == LOW);  // active LOW

    if (btnDown && !wasDown) {
        wasDown      = true;
        pressStartMs = millis();
        holdFired    = false;
    }
    // Detect 2-second hold while button is still pressed
    if (wasDown && btnDown && !holdFired && (millis() - pressStartMs >= HOLD_MS)) {
        holdFired  = true;
        wasDown    = false;
        clickCount = 0;
        return BTN_HOLD_2S;
    }
    // Button released — count the click
    if (!btnDown && wasDown) {
        wasDown       = false;
        lastReleaseMs = millis();
        if (!holdFired) clickCount++;
    }
    // Evaluate click burst after the inter-click silence window expires
    if (!btnDown && !wasDown && clickCount > 0 && (millis() - lastReleaseMs >= CLICK_GAP)) {
        uint8_t n  = clickCount;
        clickCount = 0;
        if      (n == 1) return BTN_SINGLE;
        else if (n == 3) return BTN_TRIPLE;
        else if (n >= 4) return BTN_QUAD;
    }
    return BTN_NONE;
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
    // VTOR persists across NVIC_SystemReset() — only relevant when SVC dispatcher
    // is active (ENABLE_BLE). Safe to reset regardless.
#if ENABLE_BLE
    SCB->VTOR = 0x27000UL;
    __DSB(); __ISB();
#endif

    // 1 LED blink = firmware is alive, setup() entered (visible before Serial init).
    // If you see no LED activity at all after reset, the firmware is not running.
    BLINK_LED(1);

    // USB serial (debug / phone comms)
    Serial.begin(115200);

    // Show display content immediately — before waiting for USB CDC so there
    // is always visual feedback even on fast crash/reset loops.
    initDisplay();
    dspStatus("Booting...", DUCK_NAME);

    // USB CDC will only enumerate AFTER Bluefruit.begin() starts the SD event task.
    // Do not wait for it here — we initialise CDP first, BLE last.
    Serial.println("[BOOT] Seeed Wio Tracker L1 Pro MamaDuck"); Serial.flush();
#if ENABLE_BLE
    {
        uint8_t gp = (uint8_t)(NRF_POWER->GPREGRET & 0xFFu);
        if (gp & 0x80u) {
            uint8_t step = gp & 0x7Fu;
            Serial.printf("[BLE-PREV-GP] step=0x%02X (%u)\n", step, step); Serial.flush();
            NRF_POWER->GPREGRET = 0;
        }
    }
    if (gFaultMagic == FAULT_MAGIC) {
        gFaultMagic = 0;
        Serial.printf("[FAULT-PREV] PC=0x%08X LR=0x%08X CFSR=0x%08X HFSR=0x%08X\n",
                      gFaultPC, gFaultLR, gFaultCFSR, gFaultHFSR); Serial.flush();
    }
#endif // ENABLE_BLE
    Serial.println("[SETUP] USB serial ready"); Serial.flush();

    // ADC resolution — must be called before any analogRead().
    // ADC_RESOLUTION = 14 is defined in variant.h; the BSP defaults to 10 if
    // this call is omitted.  14-bit gives full-scale 16383 (0x3FFF).
    analogReadResolution(ADC_RESOLUTION);

    // LED + Button
    pinMode(PIN_LED,     OUTPUT);
    digitalWrite(PIN_LED, LOW);
    pinMode(PIN_BUTTON1, INPUT_PULLUP);   // active LOW

    // GPS — wake the L76KB before starting Serial1
    pinMode(PIN_GPS_WAKEUP, OUTPUT);
    digitalWrite(PIN_GPS_WAKEUP, HIGH);   // STDBY_N high = active
    Serial1.begin(GPS_BAUDRATE);
    Serial.println("[GPS] Serial1 started at " + String(GPS_BAUDRATE) + " baud"); Serial.flush();

    // initDisplay() + dspStatus("Booting...") were moved to before the Serial
    // wait at the top of setup() so the display always shows content on boot.

    // ── CDP init (LoRa / routing / storage) ─────────────────────────────────
    // Do this BEFORE BLE so LoRa and I2C are stable before sd_softdevice_enable.
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

#if ENABLE_BLE
    // ── BLE last (SoftDevice + NUS advertising) ───────────────────────────────
    display.setI2CAddress(0x3D << 1);
    display.begin();
    display.setContrast(255);
    display.setPowerSave(0);
    display.setFont(u8g2_font_6x10_tf);
    dspStatus("BLE init", DUCK_NAME);

    Serial.println("[SETUP] calling install_svc_dispatcher..."); Serial.flush();
    install_svc_dispatcher();
    Serial.println("[SETUP] install_svc_dispatcher done"); Serial.flush();

    Serial.println("[SETUP] calling setupBLE..."); Serial.flush();
    setupBLE();
    Serial.println("[SETUP] setupBLE returned"); Serial.flush();

    // Third display init: after SoftDevice is fully up.
    display.setI2CAddress(0x3D << 1);
    display.begin();
    display.setContrast(255);
    display.setPowerSave(0);
    display.setFont(u8g2_font_6x10_tf);

    if (gBleStep != 99u) {
        Serial.printf("[BLE] init failed at step 0x%02X\n", (unsigned)gBleStep); Serial.flush();
        char dspFail[22];
        snprintf(dspFail, sizeof(dspFail), "step=%02X", (unsigned)gBleStep);
        dspStatus("BLE FAIL", dspFail);
        BLINK_LED(8);
    } else {
        BLINK_LED(3); BLINK_LED(3);
        dspStatus("BLE OK", DUCK_NAME);
        Serial.println("[BLE] init OK"); Serial.flush();
    }
    delay(300);
#else
    // BLE disabled — re-init display after CDP/LoRa are stable then show ready.
    // Only reinit if the first initDisplay() succeeded (address already known).
    if (gDisplayOk) {
        display.begin();
        display.setContrast(255);
        display.setPowerSave(0);
        display.setFont(u8g2_font_6x10_tf);
        dspStatus("CDP Ready", DUCK_NAME);
    }
    BLINK_LED(3);
#endif // ENABLE_BLE
    Serial.println("[MAMA] Setup complete"); Serial.flush();
    Serial.println("CDK:ID,VALUE:" DUCK_NAME);
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
            digitalWrite(PIN_LED, HIGH); delay(50);
            digitalWrite(PIN_LED, LOW);  delay(50);
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
    if (!Serial && millis() - loopFirstMs < 5000) return;

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
            delay(3000);
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
            dspStrCenter(26, "GPS TELEFON");
            dspStrCenter(38, "TIADA ISYARAT");
            dspEnd();
            delay(2000);
        } else {
            dspStr(0, 14, gpsLoraOk ? "BERJAYA HANTAR GPS!" : "GAGAL HANTAR GPS!");
            dspStr(0, 28, ("LAT:" + String(phoneGpsLatBuf)).c_str());
            dspStr(0, 40, ("LNG:" + String(phoneGpsLngBuf)).c_str());
            dspStr(0, 52, "SRC:TELEFON");
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
        dspStrCenter(26, "USB BERSIRI");
        dspStrCenter(38, "TERSAMBUNG!");
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
        dspStrCenter(26, "USB BERSIRI");
        dspStrCenter(38, "TERPUTUS");
        dspEnd();
        delay(2000);
        displayHome();
    }

    // SOS acknowledgement from operator.
    if (sosAckDisplayPending) {
        sosAckDisplayPending = false;
        displayEnabled = true;
        dspPowerSave(0);
        dspBegin();
        dspStrRight(0, idBuf);
        dspStrCenter(16, "SOS DITERIMA!");
        dspStrCenter(28, "BANTUAN SEDANG");
        dspStrCenter(40, "DIHANTAR");
        dspEnd();
        blinkLed(3);
        messagePending          = false;
        emergencyDisplayPending = true;
    }

    // Auto-dismiss received message after MESSAGE_DISPLAY_MS.
    if (messagePending && (millis() - messagePendingMs >= MESSAGE_DISPLAY_MS)) {
        messagePending = false;
        displayEnabled = true;
        dspPowerSave(0);
        displayHome();
    }

    // Auto-refresh home screen.
    if (displayEnabled && !phoneGpsDisplayPending
        && !usbConnectDisplayPending && !messagePending && !emergencyDisplayPending
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

        if (!gotGps && isPhoneConnected()) {
            if (phoneGpsLatBuf[0] == '\0') {
                dspBegin();
                dspStrRight(0, idBuf);
                dspStrCenter(22, "MEMINTA GPS");
                dspStrCenter(34, "DARIPADA TELEFON...");
                dspEnd();
                broadcast("CDK:GPSREQ");
                unsigned long waitStart = millis();
                while (phoneGpsLatBuf[0] == '\0' && millis() - waitStart < 2500) {
                    duck.run();
                    delay(50);
                }
            }
            if (phoneGpsLatBuf[0] != '\0') {
                gpsLat = String(phoneGpsLatBuf);
                gpsLng = String(phoneGpsLngBuf);
                if (phoneGpsAltBuf[0] != '\0') gpsAlt = String(phoneGpsAltBuf);
                if (phoneGpsSpdBuf[0] != '\0') gpsSpd = String(phoneGpsSpdBuf);
                if (phoneGpsHdgBuf[0] != '\0') gpsHdg = String(phoneGpsHdgBuf);
            }
        }

        dspBegin();
        dspStrRight(0, idBuf);
        displayBatt();
        dspStrCenter(22, "SEDANG HANTAR");
        dspStrCenter(34, "ISYARAT KECEMASAN...");
        dspEnd();

        sendEmergency(gpsLat, gpsLng, gpsAlt, gpsSpd, gpsHdg,
                      /* gpsFromPhone= */ !gotGps && gpsLat.length() > 0);
    }

    if (btn == BTN_SINGLE) {
        if (emergencyDisplayPending) {
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

    if (btn == BTN_TRIPLE) {
        duck.sendData(topics::status, std::string("MSG,SRC:DEVICE,TEXT:Roger"));
        broadcast("CDK:ACK,ID:ROGER");
        dspPowerSave(0);
        displayEnabled = true;
        dspBegin();
        dspStrRight(0, idBuf);
        dspStrCenter(28, "ROGER DIHANTAR!");
        dspEnd();
        delay(2000);
        displayHome();
    }

    if (btn == BTN_QUAD) {
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
                dspStrCenter(26, "TARIKH/MASA");
                dspStrCenter(38, "TIADA ISYARAT");
            }
            dspEnd();
            delay(2500);
        } else {
            dspStrCenter(28, gpsModuleDetected ? "GPS: MODUL AKTIF" : "GPS: TIADA MODUL");
            dspStrCenter(40, gpsModuleDetected ? "MENUNGGU ISYARAT..." : "(Serial1)");
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
            Serial.println("CDK:ID,VALUE:" DUCK_NAME);
            sendBattery();
            lastBattMs        = millis();
            lastUsbAnnounceMs = millis();
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

    // ── BLE incoming (line dispatched from ble_soc_task) ──────────────────────
#if ENABLE_BLE
    if (bleRxPending) {
        String line = String(bleRxLine);
        bleRxPending = false;
        if (line.startsWith("CDK:")) {
            if (!blePhoneSeen) usbConnectDisplayPending = true;
            blePhoneSeen = true;
        }
        handleFrame(line);
    }
    if (blePhoneSeen && lastBleRxMs > 0 && millis() - lastBleRxMs > USB_IDLE_TIMEOUT_MS) {
        blePhoneSeen = false;
        usbDisconnectDisplayPending = true;
    }
#endif // ENABLE_BLE

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
                broadcast("CDK:SOS_ACK,TEXT:SOS DITERIMA");
                break;
            }
            dspPowerSave(0);
            displayMessage(message);
            emergencyDisplayPending = true;
            displayEnabled          = true;
            snprintf(replyMsg, sizeof(replyMsg), "MSG_READ:TEXT:%s", message.c_str());
            duck.sendData(22, replyMsg);
            blinkLed(1);
            broadcast(String("CDK:MSG,TEXT:") + message);
            break;

        case 23:
            flashLED();
            broadcast(String("CDK:MSG,TEXT:") + message);
            duck.sendData(23, "ALERT_ACK");
            break;

        case 24:
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
                dspStr(0, 14, "MENGHANTAR DATA GPS");
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
                        dspStr(0, 14, "MENGHANTAR DATA GPS");
                        dspStr(0, 28, ("LAT:" + String(phoneGpsLatBuf)).c_str());
                        dspStr(0, 42, ("LNG:" + String(phoneGpsLngBuf)).c_str());
                    } else {
                        dspStrCenter(28, "MEMINTA DATA GPS");
                        dspStrCenter(40, "DARIPADA TELEFON...");
                    }
                    dspEnd();
                    if (gpsReqDeferredSendMs == 0) gpsReqDeferredSendMs = millis() + 400;
                } else {
                    dspStrCenter(28, "TIADA TELEFON");
                    dspStrCenter(40, "TIADA DATA GPS");
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
    // Signal / TX status
    String sigStr;
    if      (lastSignalPct >= 0 && lastSignalPct <= 25) sigStr = "SIG: LEMAH ("   + String(lastSignalPct) + "%)";
    else if (lastSignalPct >= 0 && lastSignalPct <= 50) sigStr = "SIG: CUKUP ("   + String(lastSignalPct) + "%)";
    else if (lastSignalPct >= 0 && lastSignalPct <= 75) sigStr = "SIG: KUAT ("    + String(lastSignalPct) + "%)";
    else if (lastSignalPct >  75)                       sigStr = "SIG: SG.KUAT (" + String(lastSignalPct) + "%)";
    else if (lastTxResult == 0)                         sigStr = "BERJAYA HANTAR";
    else if (lastTxResult >  0)                         sigStr = "GAGAL HANTAR";
    else                                                sigStr = "SIG: TIADA ISYARAT";
    dspStrCenter(12, sigStr.c_str());
    dspStrCenter(26, "TEKAN BUTANG ATAS");
    dspStrCenter(38, "SELAMA DUA SAAT UTK");
    dspStrCenter(50, "ISYARAT KECEMASAN");
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
    messagePending    = true;
    messagePendingMs  = millis();
    displayEnabled    = true;
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
        digitalWrite(PIN_LED, HIGH);
        delay(200);
        digitalWrite(PIN_LED, LOW);
        delay(200);
    }
}

void blinkLed(int times) {
    for (int n = 0; n < times; n++) {
        digitalWrite(PIN_LED, HIGH);
        { unsigned long t = millis(); while (millis() - t < 200) { duck.run(); delay(5); } }
        digitalWrite(PIN_LED, LOW);
        { unsigned long t = millis(); while (millis() - t < 200) { duck.run(); delay(5); } }
    }
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

    std::string loraMsg = "SOS,SRC:DEVICE,ID:" DUCK_NAME;
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
        String sosFrame = String("CDK:SOS,SRC:DEVICE,ID:" DUCK_NAME
                                 ",LAT:") + (hasGps ? lat : "none") +
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
        dspStrCenter(22, hasGps ? "BERJAYA HANTAR" : "BERJAYA HANTAR");
        dspStrCenter(34, hasGps ? "ISYARAT KECEMASAN" : "ISYARAT KECEMASAN");
        dspStrCenter(46, hasGps ? "DENGAN GPS!"     : "");
        dspEnd();
        blinkLed(2);
        {
            unsigned long endMs = millis() + 2000UL;
            while (millis() < endMs) { duck.run(); delay(10); }
        }
        displayHome();
    } else {
        dspBegin();
        dspStrRight(0, idBuf);
        displayBatt();
        dspStrCenter(22, "RALAT. TIDAK BOLEH");
        dspStrCenter(34, "HANTAR ISYARAT");
        dspStrCenter(46, "KECEMASAN");
        dspEnd();
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
    bleSendLine(payload);   // no-op if BLE is not connected
}

// ── Frame dispatcher ──────────────────────────────────────────────────────────
void handleFrame(const String& line) {
    if (!line.startsWith("CDK:")) return;
    broadcast("CDK:ID,VALUE:" DUCK_NAME);
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
    dspStrCenter(22, "SEDANG HANTAR");
    dspStrCenter(34, "ISYARAT KECEMASAN...");
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
    dspStrCenter(22, "BERJAYA HANTAR");
    dspStrCenter(34, "ISYARAT KECEMASAN");
    dspStrCenter(46, "DENGAN GPS!");
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
    dspStrCenter(22, "MESEJ TELAH DIHANTAR!");
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
