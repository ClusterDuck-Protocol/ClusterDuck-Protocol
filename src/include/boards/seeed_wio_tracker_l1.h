#pragma once

/*
 * BOARD "Seeed Wio Tracker L1 Pro"
 * nRF52840 + SX1262 + SH1106 OLED + L76KB GPS
 *
 * Board define set by boards/seeed_wio_tracker_l1.json:
 *   ARDUINO_SEEED_WIO_TRACKER_L1
 *
 * Verify all pin numbers against your hardware schematic.
 * Pin numbers are Arduino (variant) indices, not raw P0.xx/P1.xx pads.
 */
#if defined(ARDUINO_SEEED_WIO_TRACKER_L1)

#define CDP_BOARD_NAME "Seeed Wio Tracker L1 Pro"

// ── Radio ─────────────────────────────────────────────────────────────────────
#define CDPCFG_RADIO_SX1262

// SX1262 SPI chip-select / control lines (Arduino pin numbers from variant.h)
#define CDPCFG_PIN_LORA_CS      4    // D4  = P1.14 — SX1262 NSS
#define CDPCFG_PIN_LORA_RST     2    // D2  = P1.07 — SX1262 NRESET
#define CDPCFG_PIN_LORA_DIO1    1    // D1  = P0.07 — SX1262 DIO1 (IRQ)
#define CDPCFG_PIN_LORA_DIO0    1    // alias — CDP expects this name for SX1262
#define CDPCFG_PIN_LORA_BUSY    3    // D3  = P1.10 — SX1262 BUSY

// TCXO reference oscillator on this board (1.8 V).
// Enables lora.setTCXO() in DuckLoRa.cpp.
#define CDPCFG_LORA_TCXO_VOLTAGE  1.8f

// DIO2 drives the on-board RF switch — enable it in DuckLoRa.cpp.
#define CDPCFG_LORA_DIO2_RF_SWITCH

// ── Display ───────────────────────────────────────────────────────────────────
// The SH1106 OLED is driven directly by the example sketch using U8g2.
// Defining CDPCFG_OLED_NONE prevents DuckDisplay.cpp from instantiating an
// SSD1306 object (which would fail to compile on nRF52).
#define CDPCFG_OLED_NONE

// ── WiFi ──────────────────────────────────────────────────────────────────────
// nRF52840 has no WiFi; disabling prevents WiFi.h / EEPROM.h from being pulled
// into the build via CDP.h → DuckWifi.h.
#define CDPCFG_WIFI_NONE


#endif  // ARDUINO_SEEED_WIO_TRACKER_L1
