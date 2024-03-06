#pragma once


/*
 * BOARD "ttgo lora" and "heltec lora" v1
 *
 * heltec pcb is white, ttgo pcb has heltec+ttgo markings
 * Top left in this picture: https://github.com/lewisxhe/TTGO-LoRa-Series
 * (antenna attached with small cable, "chiseled" edges next to buttons
 * pio: board = ttgo-lora32-v1
 */
#if defined(ARDUINO_TTGO_LoRa32_V1)
#define CDP_BOARD_NAME "TTGO LoRa32 V1 (SX1276)"


#define CDPCFG_PIN_LED1 25

// LoRa configuration
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_DIO1 -1
#define CDPCFG_PIN_LORA_RST 23

// OLED display settings
//#define CDPCFG_PIN_OLED_ROTATION U8G2_R0
#define CDPCFG_PIN_OLED_CLOCK 22
#define CDPCFG_PIN_OLED_DATA 21
#define CDPCFG_PIN_OLED_RESET 16

#endif
