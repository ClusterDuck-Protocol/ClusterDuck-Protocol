/**
 * @file cdpcfg.h
 * @brief CDP central compile-time configuration file.
 *
 * - `cdpcfg-pre.h` at the beginning
 * - `cdpcfg-post.h` at the end
 *
 * To customize your build, you could ...
 * - Edit this file or
 * - Copy it to `cdpcfg-pre.h` and edit _that_ or
 * - Create a from-scratch `cdpcfg-pre.h` that just overrides f.ex. the board
 * defines or
 * - Create a `cdpcfg-post.h` to undef/define just parts
 *
 * @version
 * @date 2020-09-16
 *
 * @copyright
 *
 */

// preload optional pre-cfg
#ifndef CRPCFG_PRE
#define CRPCFG_PRE
#if __has_include("cdpcfg-pre.h")
#include "cdpcfg-pre.h"
#endif
#endif

// this is the actual main configuration section
#ifndef CDPCFG
#define CDPCFG


/*
 * HARDWARE SECTION // BOARD PINS
 * the ARDUINO_* defs are set by the arduino build env
 */


/*
 * BOARD "ttgo lora" and "heltec lora" v1
 *
 * heltec pcb is white, ttgo pcb has heltec+ttgo markings
 * Top left in this picture: https://github.com/lewisxhe/TTGO-LoRa-Series
 * (antenna attached with small cable, "chiseled" edges next to buttons
 * pio: board = ttgo-lora32-v1
 */
#if defined(ARDUINO_TTGO_LoRa32_V1)
#include "boards/ttgo-lora32-v1.h"


/*
 * BOARD "ttgo lora v2"
 *
 * Bottom right in this picture: https://github.com/lewisxhe/TTGO-LoRa-Series
 * (antenna attached with small cable)
 * pio: board = ttgo-lora32-v2
 */
#elif defined(ARDUINO_TTGO_LoRa32_V2)
#include "boards/ttgo-lora32-v2.h"


/*
 * BOARD "ttgo lora v2.1.6"
 *
 * Top right in this picture: https://github.com/lewisxhe/TTGO-LoRa-Series
 * (antenna attached directly to port on board, sometimes the boards read T3_V1.6.1 or T3_V1.6)
 * pio: board = ttgo_lora32_v21
 */
#elif defined(ARDUINO_TTGO_LoRa32_v21new)
#include "boards/ttgo-lora32-v21.h"


/*
 * BOARD "ttgo t-beam"
 *
 * http://www.lilygo.cn/prod_view.aspx?TypeId=50060&Id=1237&FId=t3:50060:3
 * (GPS module and 18650 holder)
 * pio: board = ttgo-t-beam
 */
#elif defined(ARDUINO_TBeam)
#include "boards/ttgo-t-beam.h"


/*
 * BOARD "heltec wireless stick"
 * http://www.heltec.cn/project/wireless-stick/
 * pio: board = heltec_wireless_stick
 */
#elif defined(ARDUINO_heltec_wireless_stick)
#include "boards/heltec_wireless_stick.h"


/*
 * BOARD "heltec wireless stick lite"
 * ARDUINO BOARD PACKAGE URL:
 * https://resource.heltec.cn/download/package_heltec_esp32_index.json
 * DOCS:
 * http://www.heltec.cn/project/wireless-stick-lite/
 * pio: board = WIRELESS_STICK_LITE
 */
#elif defined(ARDUINO_heltec_wireless_stick_LITE)
#include "boards/heltec_wireless_stick_lite.h"



/**
 * BOARD "ARDUINO_WIFI_LORA_32_V3"
 * https://heltec.org/project/wifi-lora-32-v3/
 * pio: board = heltec_wifi_lora_32_V3
*/

#elif defined(ARDUINO_WIFI_LORA_32_V3)
#include "boards/heltec_wifi_lora_32_V3.h"

/*
 * BOARD "Heltec Cube Cell Board ASR6501 with SX1262"
 * https://heltec.org/project/htcc-ab01/
 * pio: board = cubecell_board
 */
#elif defined(CubeCell_Board) || defined(CubeCell_Board_V2) || defined(CubeCell_BoardPlus)
#include "boards/cubecell_board.h"


/*
 * BOARD "Heltec Cube Cell Board ASR6501 with SX1262" =====
 * https://heltec.org/project/htcc-ab01/
 * pio: board = ???
 */
#elif defined(CubeCell_GPS)
// Uncomment this to enable the OLED display
//#define ENABLE_DISPLAY
#include "boards/cubecell_gps.h"


/*
 * BOARD "rocket scream Mini Ultra Pro v3"
 * This is not an official ARDUINO_* but uses the same bootloader as the SAMD21 based Arduino Zero
 * https://www.rocketscream.com/blog/docs-item/mini-ultra-pro-hookup-guide/
 * pio: board = arduino_zero
 */
#elif defined(ARDUINO_SAMD_ZERO)
#include "boards/arduino_zero.h"


/*
 * BOARD "sparkfun lora gateway 1-channel"
 * this is not an official ARDUINO_* define since this board doesnt have one
 * https://www.sparkfun.com/products/15006
 * pio: board = sparkfun_lora_gateway_1-channel
 */
#elif defined(SPARKFUN_LGW1C)
#include "boards/sparkfun_lora_gateway_1.h"


/*
 * BOARD "pycom lopy"
 * https://docs.pycom.io/datasheets/development/lopy/
 * pio: board = lopy
 */
#elif defined(ARDUINO_LoPy)
#include "boards/lopy.h"


/*
 * BOARD "pycom lopy4"
 * https://docs.pycom.io/datasheets/development/lopy4/
 * pio: board = lopy4
 */
#elif defined(ARDUINO_LoPy4)
#include "boards/lopy4.h"


#else // Default to WIFI_LORA_32_V2 board
#if !defined(ARDUINO_heltec_wifi_lora_32_V2)
#warning "NO BOARD DEFINED, DEFAULTING TO HELTEC v2"
#define CDPCFG_BOARD_DEFAULT
#endif

/*
 * BOARD "heltec lora v2"
 * https://heltec.org/project/wifi-lora-32/
 * pio: board = heltec_wifi_lora_32_V2
 */
#include "boards/heltec_wifi_lora_32_V2.h"

#endif // Board definitions


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
#define CHANNEL_1 915.0
#define CHANNEL_2 914.0
#define CHANNEL_3 913.0
#define CHANNEL_4 912.0
#define CHANNEL_5 911.0
#define CHANNEL_6 910.0

/// Default LoRa Module supported chipset when using the RadioLib library
#if !defined(CDPCFG_LORA_CLASS) && !defined(CDPCFG_HELTEC_CUBE_CELL)
#define CDPCFG_LORA_CLASS SX1276
#endif

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

#endif // CDPCFG

// append optional post-cfg
#ifndef CRPCFG_POST
#define CRPCFG_POST
#if __has_include("cdpcfg-post.h")
#include "cdpcfg-post.h"
#endif
#endif
