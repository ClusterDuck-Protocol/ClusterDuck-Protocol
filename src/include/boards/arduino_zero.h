#pragma once

/*
 * BOARD "rocket scream Mini Ultra Pro v3"
 * This is not an official ARDUINO_* but uses the same bootloader as the SAMD21 based Arduino Zero
 * https://www.rocketscream.com/blog/docs-item/mini-ultra-pro-hookup-guide/
 */
#if defined(ARDUINO_SAMD_ZERO)

#define CDP_BOARD_NAME "Mini Ultra Pro v3" 
#define CDPCFG_LORA_CLASS SX1276

// LoRa configuration (HopeRF RFM95W chip)
#define CDPCFG_PIN_LORA_CS      5
#define CDPCFG_PIN_LORA_DIO0    2
#define CDPCFG_PIN_LORA_DIO1    6
#define CDPCFG_PIN_LORA_DIO2    -1 // Unused
#define CDPCFG_PIN_LORA_RST     3

// OLED display settings
#define CDPCFG_OLED_NONE

// WiFi module
#define CDPCFG_WIFI_NONE

// Required for Serial on Zero based boards
#define Serial SERIAL_PORT_USBVIRTUAL

#endif