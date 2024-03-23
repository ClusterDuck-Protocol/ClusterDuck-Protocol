/**
 * @file cdpcfg.h
 * @brief CDP central compile-time configuration file.
 *
 * @version
 * @date 2020-09-16
 *
 * @copyright
 *
 */

#ifndef CDPCFG_H
#define CDPCFG_H

#include "boards/heltec_wifi_lora_32_V3.h"
#include "boards/heltec_wifi_lora_32_V2.h"
#include "boards/lilygo_t_beam_sx1262.h"
#include "boards/ttgo_t_beam_v1_sx1276.h"
#include "boards/heltec_cubecell_board.h"
#include "boards/heltec_cubecell_gps.h"

// version definitions
#define CDP_VERSION_MAJOR  4
#define CDP_VERSION_MINOR  1
#define CDP_VERSION_PATCH  0

#define CDP_VERSION ((((CDP_VERSION_MAJOR) << 16) | ((CDP_VERSION_MINOR) << 8) | (CDP_VERSION_PATCH)))

#ifdef CDPCFG_RADIO_SX1262
  #define CDPCFG_LORA_CLASS SX1262
#else
  #define CDPCFG_LORA_CLASS SX1276
#endif

/**
 * @brief Non board specific configuration
 */

//Default Username and Password
#define CDPCFG_UPDATE_USERNAME "user"
#define CDPCFG_UPDATE_PASSWORD "pass"

#define CDPCFG_EEPROM_CRED_MAX 32
#define CDPCFG_EEPROM_WIFI_USERNAME 0
#define CDPCFG_EEPROM_WIFI_PASSWORD 32
#define CDPCFG_EEPROM_CONTROL_USERNAME 64
#define CDPCFG_EEPROM_CONTROL_PASSWORD 96
#define CDPCFG_EEPROM_CHANNEL_VALUE 128

/// Serial Console Baud Rate
#define CDPCFG_SERIAL_BAUD 115200

// Access point IP adress

#define CDPCFG_AP_IP1 192
#define CDPCFG_AP_IP2 168
#define CDPCFG_AP_IP3 1
#define CDPCFG_AP_IP4 1

/** @brief Asyncwebserver Port */
#define CDPCFG_WEB_PORT 80

/// Frequency Range. Set for US Region 915.0Mhz
#define CDPCFG_RF_LORA_FREQ 915.0
#define CDPCFG_RF_LORA_FREQ_HZ 915000000
/// Bandwidth. Default is 125Mhz
#define CDPCFG_RF_LORA_BW 125.0
/// Spread Factor
#define CDPCFG_RF_LORA_SF 7
/// Transmit Power
#define CDPCFG_RF_LORA_TXPOW 20
/// Antenna Gain correction
#define CDPCFG_RF_LORA_GAIN 0

/// CDP message buffer max length
#define CDPCFG_CDP_BUFSIZE 256
/// CDP UUID generator max length
#define CDPCFG_UUID_LEN 8

/// CDP chat circular buffer size
#define CDPCFG_CDP_CHATBUF_SIZE 15

/// CDP ALIVE timer duration in milliseconds
#define CDPCFG_MILLIS_ALIVE 1800000
/// CDP REBOOT timer duration in milliseconds
#define CDPCFG_MILLIS_REBOOT 43200000

/// CDP RGB Led RED Pin default value
#define CDPCFG_PIN_RGBLED_R 25
/// CDP RGB Led GREEN Pin default value
#define CDPCFG_PIN_RGBLED_G 4
/// CDP RGB Led BLUE Pin default value
#define CDPCFG_PIN_RGBLED_B 2

/// CDP Channel Frequencies
#define CDPCFG_RADIO_CHANNEL_1 CDPCFG_RF_LORA_FREQ
#define CDPCFG_RADIO_CHANNEL_2 914.0
#define CDPCFG_RADIO_CHANNEL_3 913.0
#define CDPCFG_RADIO_CHANNEL_4 912.0
#define CDPCFG_RADIO_CHANNEL_5 911.0
#define CDPCFG_RADIO_CHANNEL_6 910.0

#ifndef CDPCFG_OLED_CLASS
#if defined(CDPCFG_OLED_NONE)
// do nothing
#elif defined(CDPCFG_OLED_64x32)
// smaller displays
#define CDPCFG_OLED_CLASS U8G2_SSD1306_64X32_NONAME_F_SW_I2C
#else
// classic default
#define CDPCFG_OLED_CLASS U8G2_SSD1306_128X64_NONAME_F_SW_I2C
#endif // CDPCFG_OLED_NONE
#endif // CDPCFG_OLED_CLASS

#endif // CDPCFG_H