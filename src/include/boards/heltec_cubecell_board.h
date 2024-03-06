#pragma once

/*
 * BOARD "Heltec Cube Cell Board ASR6501 with SX1262"
 * https://heltec.org/project/htcc-ab01-v2/
 */

#if defined(CubeCell_Board) || defined(CubeCell_Board_V2) || defined(CubeCell_BoardPlus)

#define CDP_BOARD_NAME "Heltec CubeCell Series"
#define CDPCFG_RADIO_SX1262

// Lora configurations
#define CDPCFG_PIN_LORA_CS      35
#define CDPCFG_PIN_LORA_DIO0    39
#define CDPCFG_PIN_LORA_BUSY    39
#define CDPCFG_PIN_LORA_DIO1    38
#define CDPCFG_PIN_LORA_DIO2    -1
#define CDPCFG_PIN_LORA_RST     47

// Oled Display settings
#define CDPCFG_OLED_NONE

// Wifi module
#define CDPCFG_WIFI_NONE

#endif
