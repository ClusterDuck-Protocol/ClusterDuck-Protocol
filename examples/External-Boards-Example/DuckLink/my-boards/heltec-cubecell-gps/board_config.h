#pragma once

#define CDP_BOARD_NAME "Heltec CubeCell GPS" 
#define CDPCFG_RADIO_SX1262

// Uncomment this to enable the OLED display
//#define ENABLE_DISPLAY

#define CDPCFG_PIN_LORA_CS      35
#define CDPCFG_PIN_LORA_DIO0    39 
#define CDPCFG_PIN_LORA_BUSY    39
#define CDPCFG_PIN_LORA_DIO1    38
#define CDPCFG_PIN_LORA_DIO2    -1
#define CDPCFG_PIN_LORA_RST     47


// Oled Display settings
#ifdef ENABLE_DISPLAY
#define CDPCFG_PIN_OLED_CLOCK   15
#define CDPCFG_PIN_OLED_DATA    4
#define CDPCFG_PIN_OLED_RESET   16
#else
#define CDPCFG_OLED_NONE
#endif

// Wifi module
#define CDPCFG_WIFI_NONE