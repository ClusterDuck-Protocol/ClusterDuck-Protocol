#pragma once

/*
 * BOARD "T_Beam"
 *
 * https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/tree/master
 * (GPS module and 18650 holder)
 */
#if defined(ARDUINO_T_Beam)

#define CDP_BOARD_NAME "LilyGO T_Beam SX1262"
#define CDPCFG_RADIO_SX1262

#define CDPCFG_PIN_BAT 35 
#define CDPCFG_BAT_MULDIV 200 / 100 
#define CDPCFG_PIN_LED1 25

//GPS configuration
#define CDPCFG_GPS_RX 34
#define CDPCFG_GPS_TX 12

// LoRa configuration
#define CDPCFG_PIN_LORA_CS      18
#define CDPCFG_PIN_LORA_RST     23
#define CDPCFG_PIN_LORA_DIO0    26
#define CDPCFG_PIN_LORA_DIO1    33
#define CDPCFG_PIN_LORA_BUSY    32

// Oled Display settings
#define CDPCFG_OLED_NONE
#endif