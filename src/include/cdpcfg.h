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

#ifdef CDP_EXTERNAL_BOARD
#include "cdp_external_board.h"
#else
#include "boards/heltec_wifi_lora_32_V3.h"
#include "boards/heltec_wifi_lora_32_V2.h"
#include "boards/lilygo_t_beam_sx1262.h"
#include "boards/ttgo_t_beam_v1_sx1276.h"
#include "boards/lilygo_t_beam_supreme_sx1262.h"
#include "boards/lilygo_sim7000g_lora.h"
#include "boards/seeed_wio_tracker_l1.h"
#endif

// version definitions
#define CDP_VERSION_MAJOR  5
#define CDP_VERSION_MINOR  0
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

/// Frequency Range. Set for US Region 915.0Mhz, Asia Region AS923 is below 923.0Mhz
#define CDPCFG_RF_LORA_FREQ 922.8f
#define CDPCFG_RF_LORA_FREQ_HZ 922800000
/// Bandwidth. Default is 125Mhz
#define CDPCFG_RF_LORA_BW 125.0f

/**
 * @brief Uplink channel spreading pool (AS923, 200 kHz spacing).
 *
 * When a duck originates a packet addressed directly to PapaDuck, it
 * transmits on a randomly selected channel from this pool instead of the
 * shared mesh channel (CDPCFG_RADIO_CHANNEL_1 / CDPCFG_RF_LORA_FREQ). This
 * spreads last-hop uplink traffic across all of PapaDuck's SX1302
 * multi-SF demodulators, reducing collisions. Relayed packets always stay
 * on the shared mesh channel. See docs/uplink-channel-spreading.md.
 */
#define CDPCFG_RADIO_CHANNEL_1 922.8f
#define CDPCFG_RADIO_CHANNEL_2 922.6f
#define CDPCFG_RADIO_CHANNEL_3 922.4f
#define CDPCFG_RADIO_CHANNEL_4 922.2f
#define CDPCFG_RADIO_CHANNEL_5 922.0f
#define CDPCFG_RADIO_CHANNEL_6 921.8f
#define CDPCFG_RADIO_CHANNEL_7 921.6f
#define CDPCFG_RADIO_CHANNEL_8 921.4f

#define CDPCFG_UPLINK_CHANNEL_POOL { \
  CDPCFG_RADIO_CHANNEL_1, CDPCFG_RADIO_CHANNEL_2, CDPCFG_RADIO_CHANNEL_3, \
  CDPCFG_RADIO_CHANNEL_4, CDPCFG_RADIO_CHANNEL_5, CDPCFG_RADIO_CHANNEL_6, \
  CDPCFG_RADIO_CHANNEL_7, CDPCFG_RADIO_CHANNEL_8 }
/// Spread Factor
//#define CDPCFG_RF_LORA_SF 7
#define CDPCFG_RF_LORA_SF 7
/// Transmit Power
#define CDPCFG_RF_LORA_TXPOW 14
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

// CDP Acceptable Signal Ranges
#define RSSI_MAX (-20.0f)
#define RSSI_MIN (-131.0f)
#define SNR_MAX 11.5f
#define SNR_MIN (-11.5f)

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
