#pragma once

/*
 * BOARD "LilyGO T-SIM7000G with LoRa Hat"
 *
 * https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G
 * ESP32 board with SIM7000G cellular module and LoRa expansion hat
 */
#if defined(ARDUINO_LILYGO_T_SIM7000G)

#define CDP_BOARD_NAME "LilyGO T-SIM7000G with LoRa Hat"
#define CDPCFG_RADIO_SX1276

#define CDPCFG_PIN_BAT 35 
#define CDPCFG_BAT_MULDIV 200 / 100 
#define CDPCFG_PIN_LED1 25

// SIM7000G UART configuration
#define CDPCFG_PIN_TX 27
#define CDPCFG_PIN_RX 26
#define CDPCFG_UART_BAUD 115200
#define CDPCFG_PWR_PIN 4

// GPS configuration (using SIM7000G internal GPS)
#define CDPCFG_GPS_RX 26
#define CDPCFG_GPS_TX 27

// SD Card configuration
#define CDPCFG_SD_MISO 2
#define CDPCFG_SD_MOSI 15
#define CDPCFG_SD_SCLK 14
#define CDPCFG_SD_CS 13

// LoRa configuration (LoRa Hat pins)
#define CDPCFG_PIN_LORA_CS      5
#define CDPCFG_PIN_LORA_RST     12
#define CDPCFG_PIN_LORA_DIO0    32
#define CDPCFG_PIN_LORA_DIO1    33
#define CDPCFG_PIN_LORA_DIO2    34
#define CDPCFG_PIN_LORA_MISO    19
#define CDPCFG_PIN_LORA_MOSI    23
#define CDPCFG_PIN_LORA_SCK     18

// Display configuration (using I2C OLED if connected)
#define CDPCFG_PIN_OLED_CLOCK 22
#define CDPCFG_PIN_OLED_DATA 21
#define CDPCFG_PIN_OLED_RESET 16

#endif
