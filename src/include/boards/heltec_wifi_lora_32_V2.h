#pragma once

#if defined(ARDUINO_HELTEC_WIFI_LORA_32_V2) || defined(ARDUINO_heltec_wifi_lora_32_V2) || defined(heltec_wifi_lora_32_V2)

#define CDP_BOARD_NAME "Heltec WiFi LoRa 32 V2"

#define CDPCFG_PIN_BAT 37
#define CDPCFG_BAT_MULDIV 320 / 100

#define CDPCFG_PIN_VEXT 21

#define CDPCFG_PIN_LED1 25

// LoRa configuration
#define CDPCFG_PIN_LORA_CS      18
#define CDPCFG_PIN_LORA_DIO0    26
#define CDPCFG_PIN_LORA_DIO1    -1
#define CDPCFG_PIN_LORA_RST     14

// OLED display settings
#define CDPCFG_PIN_OLED_CLOCK   15
#define CDPCFG_PIN_OLED_DATA    4
#define CDPCFG_PIN_OLED_RESET   16
#define CDPCFG_PIN_OLED_ROTATION U8G2_R0

#endif
