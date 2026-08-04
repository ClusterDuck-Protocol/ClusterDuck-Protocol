/**
 * BLETest.ino — Minimal BLE diagnostic sketch for Seeed Wio Tracker L1 Pro
 * (nRF52840 + S140 v7.3.0, FreeRTOS + TinyUSB)
 *
 * PURPOSE: Test Bluefruit.begin() + BLE advertising in complete isolation.
 * Diagnostics use the SH1106 128×64 OLED via SOFTWARE I2C (bit-bang GPIO).
 * SW I2C bypasses the TWIM hardware and works even after sd_softdevice_enable()
 * (HW TWIM/Wire hangs post-sd_enable because EVENTS_STOPPED never fires).
 *
 * OLED: SDA = D14 = P0.06, SCL = D15 = P0.05, I2C addr 0x3D (8-bit 0x7A)
 * LED:  D11 = P1.01, active HIGH — kept as fast backup indicator
 *
 * OLED screens shown during boot:
 *   "BLE Test Init"            — entered setup()
 *   "SD IRQ handlers: …"       — sd_vt[17,24,27,36,37] with OK/!! status
 *   "SD IRQ (cont): …"         — sd_vt[41], SVC address, VTOR
 *   "begin() SUCCESS!"         — Bluefruit.begin() returned true
 *   "adv_start()…"             — last screen before SD calls; if HardFault
 *                                 occurs here, display stays on this screen
 *                                 and LED shows SOS pattern.
 *   "ADV STARTED!"             — advertising running ✓
 */

#include <Arduino.h>
#include <bluefruit.h>
#include <timers.h>
#include <U8g2lib.h>

// ── Software I2C OLED ─────────────────────────────────────────────────────────
// U8G2 SW I2C bit-bangs the clock and data lines directly via GPIO — no TWIM.
// This makes it safe to use both before AND after sd_softdevice_enable(), unlike
// the Arduino Wire library (hardware TWIM hangs post-sd_enable).
// Constructor order: (rotation, clock=SCL, data=SDA, reset)
U8G2_SH1106_128X64_NONAME_F_SW_I2C display(U8G2_R0,
    /*clock/SCL*/ 15,          // D15 = P0.05
    /*data/SDA*/  14,          // D14 = P0.06
    U8X8_PIN_NONE);
static bool s_disp_ok = false;

// Draw up to 6 text lines (font 6×10 px, baselines at y = 9,19,29,39,49,59).
static void disp(const char *l1, const char *l2 = nullptr,
                 const char *l3 = nullptr, const char *l4 = nullptr,
                 const char *l5 = nullptr, const char *l6 = nullptr) {
    if (!s_disp_ok) return;
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    if (l1) display.drawStr(0,  9, l1);
    if (l2) display.drawStr(0, 19, l2);
    if (l3) display.drawStr(0, 29, l3);
    if (l4) display.drawStr(0, 39, l4);
    if (l5) display.drawStr(0, 49, l5);
    if (l6) display.drawStr(0, 59, l6);
    display.sendBuffer();
}

// ── LED helpers (pure register writes, no FreeRTOS) ───────────────────────────
// P1.01 = D11 (orange user LED, active HIGH)
#define _LED_DIR()  (NRF_P1->DIRSET = (1u << 1))
#define _LED_ON()   (NRF_P1->OUTSET = (1u << 1))
#define _LED_OFF()  (NRF_P1->OUTCLR = (1u << 1))

// Busy-wait: calibrated from existing code (256000 ≈ 20ms → 12800/ms at 64MHz)
#define SPIN(iters)  do { for (volatile uint32_t _s=0; _s<(iters); _s++) {} } while(0)

// Blink N times with given on/off iteration counts (all busy-wait)
static void led_blink(uint8_t n, uint32_t on_iters, uint32_t off_iters) {
    _LED_DIR();
    for (uint8_t i = 0; i < n; i++) {
        _LED_ON();  SPIN(on_iters);
        _LED_OFF(); SPIN(off_iters);
    }
}
// Convenience shorthands
#define BLINK_FAST(n)   led_blink((n),  640000u,  640000u)  // ~50ms on/off
#define BLINK_MEDIUM(n) led_blink((n), 2560000u, 2560000u)  // ~200ms on/off
#define BLINK_SLOW(n)   led_blink((n), 6400000u, 6400000u)  // ~500ms on/off

// ── Fault handlers ───────────────────────────────────────────────────────────
// Override BSP's HardFault_Handler (which calls NVIC_SystemReset) with one that
// blinks SOS (3 short, 3 long, 3 short) repeatedly so we can distinguish a fault
// from a hang.  Defined extern "C" to match the weak symbol in debug.cpp.
extern "C" void HardFault_Handler(void) {
    _LED_DIR();
    while (true) {
        // 3 short (~150ms each)
        for (int i = 0; i < 3; i++) { _LED_ON(); SPIN(1920000u); _LED_OFF(); SPIN(1920000u); }
        SPIN(3840000u);
        // 3 long (~500ms each)
        for (int i = 0; i < 3; i++) { _LED_ON(); SPIN(6400000u); _LED_OFF(); SPIN(3200000u); }
        SPIN(3840000u);
        // 3 short again
        for (int i = 0; i < 3; i++) { _LED_ON(); SPIN(1920000u); _LED_OFF(); SPIN(1920000u); }
        SPIN(9600000u);  // ~750ms gap before repeating
    }
}

// ── RAM Vector Table & SVC dispatch ──────────────────────────────────────────
// The nRF52840 SoftDevice API uses SVC instructions.  The BSP's SVC_Handler
// (vPortSVCHandler, made weak in our port.c patch) only handles SVC 0 for
// FreeRTOS task start; any SD API SVC (number > 0) would corrupt task state.
//
// This STRONG SVC_Handler dispatches correctly:
//   MSP path (EXC_RETURN bit2=0)  → always SVC 0, FreeRTOS first-task start
//   PSP path, SVC 0               → FreeRTOS context restore
//   PSP path, SVC > 0             → SD's SVC handler at *(0x102C), r0=PSP preserved
//
// ble_post_sd_enable_hook() (called from bluefruit.cpp after sd_softdevice_enable)
// copies the app flash VT to g_ramVectors, overlays SD's peripheral IRQ handlers,
// stamps our SVC_Handler at slot 11, then sets SCB->VTOR = g_ramVectors.

#define VTOR_NUM_VECTORS 80U
static uint32_t g_ramVectors[VTOR_NUM_VECTORS] __attribute__((aligned(512)));

extern "C" { extern void * volatile pxCurrentTCB; }

extern "C" __attribute__((naked, used)) void SVC_Handler(void) {
    __asm volatile (
        "tst   lr, #0x04         \n"   // EXC_RETURN bit2: 0=MSP, 1=PSP
        "beq   1f                \n"   // MSP path → FreeRTOS first-task start
        "mrs   r0, psp           \n"   // r0 = PSP (SD expects r0=PSP on entry)
        "ldr   r1, [r0, #0x18]   \n"   // stacked PC (exception frame offset 24)
        "ldrb  r2, [r1, #-2]     \n"   // SVC immediate byte
        "cmp   r2, #0            \n"
        "bne   2f                \n"   // SVC > 0 → forward to SD
        "1:                      \n"   // svc0_restore (MSP path or SVC 0 via PSP)
        "ldr   r3, =pxCurrentTCB \n"
        "ldr   r1, [r3]          \n"
        "ldr   r0, [r1]          \n"   // pxCurrentTCB->pxTopOfStack
        "ldmia r0!, {r4-r11, r14}\n"
        "msr   psp, r0           \n"
        "isb                     \n"
        "mov   r0, #0            \n"
        "msr   basepri, r0       \n"
        "bx    r14               \n"
        "2:                      \n"   // to_sd: forward SVC > 0 to SD handler
        "ldr   r1, =0x102C       \n"   // SD VT slot 11 = SD's SVC handler
        "ldr   r1, [r1]          \n"
        "bx    r1               \n"   // r0=PSP preserved for SD
    );
}

// Write "0x" + 8 hex digits into out[11].  No static locals, safe anywhere.
static void fmt_hex8(char *out, uint32_t v) {
    out[0]='0'; out[1]='x';
    for (int i = 9; i >= 2; i--) {
        out[i] = "0123456789ABCDEF"[v & 0xF];
        v >>= 4;
    }
    out[10] = '\0';
}

// Called from bluefruit.cpp at each milestone inside begin().
// Updates the OLED and blinks once so we know the step was reached.
static volatile uint8_t g_ble_progress = 0;
extern "C" void ble_debug_step(uint8_t n) {
    g_ble_progress = n;
    BLINK_FAST(1);
    const char * const labels[] = {
        "", "pre-begin", "begin()", "sd_en OK",
        "hook done", "ble_en OK", "tasks+IRQ",
        "bond_init", "bond done"
    };
    char buf[] = "step N";
    buf[5] = (char)('0' + n);
    disp("BLE init:", buf, (n < 9) ? labels[n] : "?");
}

// Called from bluefruit.cpp right after sd_softdevice_enable() returns OK.
// Sets VTOR to g_ramVectors with SD peripheral IRQ handlers overlaid.
extern "C" void ble_post_sd_enable_hook(void) {
    char vbuf[11];
    uint32_t vtor_val = (uint32_t)SCB->VTOR;
    fmt_hex8(vbuf, vtor_val);
    disp("hook: post_sd_en", vbuf, "copying VT...");
    BLINK_FAST(1);

    // Copy app flash VT; fall back to known S140v7 app address if SD changed VTOR
    const uint32_t *appVT = (const uint32_t*)((vtor_val >= 0x27000UL) ? vtor_val : 0x27000UL);
    for (uint32_t i = 0; i < VTOR_NUM_VECTORS; i++) g_ramVectors[i] = appVT[i];

    // Stamp our SVC_Handler at slot 11
    g_ramVectors[11] = (uint32_t)SVC_Handler | 1u;

    // Overlay SD's peripheral IRQ handlers (entries 16+) from SD's VT at 0x1000
    const uint32_t *sd_vt = (const uint32_t*)0x1000UL;
    for (uint32_t i = 16; i < VTOR_NUM_VECTORS; i++) {
        if (i == 38u) continue;  // keep app's SD_EVT_IRQHandler at vec 38
        uint32_t h = sd_vt[i];
        if (h >= 0x1001UL && h < 0x27000UL) g_ramVectors[i] = h;
    }
    __DSB();
    SCB->VTOR = (uint32_t)g_ramVectors;
    __DSB(); __ISB();

    char psv[11], stk[11];
    fmt_hex8(psv, g_ramVectors[14]);  // PendSV_Handler
    fmt_hex8(stk, g_ramVectors[15]);  // SysTick_Handler
    disp("hook: done", vbuf, psv, stk);
    BLINK_FAST(1);
}

// ── BLE UART (NUS) ────────────────────────────────────────────────────────────
static BLEUart bleuart;
static volatile bool bleConnected = false;

static void onConnect(uint16_t /*conn_handle*/) {
    bleConnected = true;
    // 3 fast blinks on connect (runs in BLE task, not loop_task — safe)
    BLINK_FAST(3);
}
static void onDisconnect(uint16_t /*conn_handle*/, uint8_t /*reason*/) {
    bleConnected = false;
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    // Give the USB CDC host up to 500ms to open the port before setup races past
    for (uint32_t t = 0; t < 500; t++) { if (Serial) break; delay(1); }
    if (Serial) Serial.println("\n[SETUP] started");
    _LED_DIR(); _LED_ON();  // solid on while display initialises

    // ── OLED init ─────────────────────────────────────────────────────────
    // Must be first — gives visual feedback for every subsequent step.
    // setI2CAddress() must be called before begin() for non-default addresses.
    display.setI2CAddress(0x7A);   // 8-bit representation of 7-bit addr 0x3D
    s_disp_ok = display.begin();
    _LED_OFF();
    disp("BLE Test v3",
         s_disp_ok ? "OLED SW-I2C OK" : "OLED FAIL (no disp)");
    BLINK_FAST(1);  // 1 fast blink = setup() entered

    disp("Configuring BLE...", "BANDWIDTH_MAX", "starting progress", "timer...");
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

    // No VTOR write here — SVC_Handler is already live in the flash VT.
    // ble_post_sd_enable_hook() (called from begin()) activates g_ramVectors.
    BLINK_FAST(5);  // 5 fast = about to call begin()
    disp("Calling begin()...", "SVC in flash VT", "hook activates RAM VT");
    if (Serial) Serial.println("[SETUP] calling begin()");
    bool ok = Bluefruit.begin(1, 0);

    _LED_DIR(); _LED_OFF();

    if (ok) {
        disp("begin() SUCCESS!", "Configuring adv...");
        BLINK_SLOW(3);   // 3 slow = begin() OK
    } else {
        disp("begin() FAILED!", "(SOS LED = HardFault)");
        BLINK_FAST(15);
        return;
    }

    // ── Post-begin BLE configuration ──────────────────────────────────────
    Bluefruit.setTxPower(4);
    Bluefruit.setName("WIO-BLE-TEST");
    Bluefruit.Periph.setConnectCallback(onConnect);
    Bluefruit.Periph.setDisconnectCallback(onDisconnect);
    disp("begin() SUCCESS!", "setTxPower OK", "setName OK");
    BLINK_FAST(1);  // checkpoint 1

    //bleuart.begin();
    //disp("begin() SUCCESS!", "bleuart.begin OK");
    //BLINK_FAST(2);  // checkpoint 2

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.setFastTimeout(30);
    // Disable BSP's connection-LED timer (calls digitalToggle via FreeRTOS —
    // eliminate as a potential crash source during debugging).
    Bluefruit.autoConnLed(false);
    disp("Adv configured OK");
    BLINK_FAST(3);  // checkpoint 3

    // ── Advertising.start() ───────────────────────────────────────────────
    // This is the crash site: sd_ble_gap_adv_start() inside _start() triggers
    // the RADIO peripheral which fires RADIO_IRQn almost immediately.
    // If g_ramVectors[17] is wrong → Default_Handler → eventual HardFault.
    //
    // The BSP blink markers inside BLEAdvertising.cpp show:
    //   2 blinks → sd_ble_gap_adv_set_configure returned NRF_SUCCESS
    //   3 blinks → sd_ble_gap_tx_power_set    returned NRF_SUCCESS
    //   4 blinks → sd_ble_gap_adv_start       returned NRF_SUCCESS (never seen yet)
    //
    // If the display freezes on "adv_start()..." with SOS on the LED,
    // the crash is inside sd_ble_gap_adv_start itself.
    disp("adv_start()...",
         "BSP 2,3,4 blinks",
         "= SD call steps",
         "SOS LED = fault");

    bool adv_ok = Bluefruit.Advertising.start(0);

    // Checkpoint 4: start() RETURNED (true or false).
    // If we reach here and never see 4 fast blinks the crash is inside start().
    BLINK_FAST(4);

    if (adv_ok) {
        disp("ADV STARTED!", "Scan for:", "  WIO-BLE-TEST",
             "Loop: 1 blink/2s", "while advertising");
        BLINK_MEDIUM(5);  // 5 medium = advertising ✓
    } else {
        disp("adv_start FAILED", "(returned false)");
        BLINK_FAST(8);
    }
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    // Heartbeat: 1 brief blink every ~2s while advertising.
    if (Bluefruit.Advertising.isRunning()) {
        BLINK_FAST(1);
        SPIN(25600000u);  // ~2s
    } else {
        SPIN(1280000u);   // ~100ms idle spin
    }
}
