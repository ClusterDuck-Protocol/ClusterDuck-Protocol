#pragma once

/*
 * BOARD "T-Beam Supreme"
 * LilyGO T-Beam Supreme ESP32-S3 with SX1262 LoRa
 * https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t_beam_supreme/t_beam_supreme_hw.md
 * (GPS module, OLED display, and 18650 holder)
 */
#if defined(ARDUINO_LILYGO_T_BEAM_SUPREME) || defined(ARDUINO_lilygo_t_beam_supreme) || defined(ARDUINO_ESP32S3_DEV)

#define CDP_BOARD_NAME "LilyGO T-Beam Supreme ESP32-S3 SX1262"
#define CDPCFG_RADIO_SX1262

// Uncomment this to enable the OLED display
#define ENABLE_DISPLAY

// Battery monitoring (using safe ESP32-S3 ADC pin, avoiding strapping pins)
#define CDPCFG_PIN_BAT 2
#define CDPCFG_BAT_MULDIV 200 / 100

// LED (avoid strapping pins, use available GPIO)  
#define CDPCFG_PIN_LED1 48  // RGB LED pin on many ESP32-S3 boards

// GPS configuration (updated to match LilyGO T-Beam Supreme specs)
#define CDPCFG_GPS_RX 9    // GNSS RX (T-Beam Supreme standard)
#define CDPCFG_GPS_TX 8    // GNSS TX (T-Beam Supreme standard)

// LoRa configuration for SX1262 (avoiding strapping pins)
#define CDPCFG_PIN_LORA_CS      10  // LoRa CS
#define CDPCFG_PIN_LORA_RST     5   // LoRa RESET
#define CDPCFG_PIN_LORA_DIO0    -1  // Not used in SX1262
#define CDPCFG_PIN_LORA_DIO1    6   // LoRa DIO1/DIO9 (SX1262 IRQ) - Changed from GPIO1 (strapping pin)
#define CDPCFG_PIN_LORA_BUSY    4   // LoRa BUSY

// OLED display settings (corrected I2C pin assignments)
#ifdef ENABLE_DISPLAY
#define CDPCFG_PIN_OLED_CLOCK   18   // SCL (T-Beam Supreme I2C standard)
#define CDPCFG_PIN_OLED_DATA    17   // SDA (T-Beam Supreme I2C standard)  
#define CDPCFG_PIN_OLED_RESET   -1   // Usually shared reset or not needed
#define CDPCFG_PIN_OLED_ROTATION U8G2_R0

// Define the OLED class for SH1106 controller (software I2C for better reliability)
#define CDPCFG_OLED_CLASS U8G2_SH1106_128X64_NONAME_F_SW_I2C
#else
#define CDPCFG_OLED_NONE
#endif

#endif 