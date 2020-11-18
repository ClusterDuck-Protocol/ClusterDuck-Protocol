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

/**
 * BOARD "ttgo lora" and "heltec lora" v1
 *
 * heltec pcb is white, ttgo pcb has heltec+ttgo markings
 * left top+middle in this picture
 * https://github.com/Xinyuan-LilyGO/TTGO-LoRa-Series
 * pio: board = ttgo-lora32-v1
 */
#if defined(ARDUINO_TTGO_LoRa32_V1)

#define CDPCFG_PIN_LED1 25

// Lora configurations
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_RST 23

// Oled Display settings
// #define CDPCFG_PIN_OLED_ROTATION U8G2_R0
#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA 4
#define CDPCFG_PIN_OLED_RESET 16

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

/**
 * BOARD "ttgo lora v2"
 *
 * Top right in this picture
 * https://github.com/Xinyuan-LilyGO/TTGO-LoRa-Series
 * pio: board = ttgo-lora32-v2
 */
#elif defined(ARDUINO_TTGO_LoRa32_V2)

#define CDPCFG_PIN_BAT 35
#define CDPCFG_BAT_MULDIV 200 / 100

#define CDPCFG_PIN_LED1 25

// Lora configurations
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_RST 14

// Oled Display settings

#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA 4
#define CDPCFG_PIN_OLED_RESET 16
#define CDPCFG_PIN_OLED_ROTATION U8G2_R0

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

// T-Beam
 #elif defined(ARDUINO_TBeam)
  #define CDPCFG_PIN_BAT 35 
  #define CDPCFG_BAT_MULDIV 200 / 100 
  #define CDPCFG_PIN_LED1 25 
  // Lora configurations 
  #define CDPCFG_PIN_LORA_CS 18 
  #define CDPCFG_PIN_LORA_DIO0 26 
  #define CDPCFG_PIN_LORA_RST 14 
  // Oled Display settings 
  #define CDPCFG_PIN_OLED_CLOCK 22 
  #define CDPCFG_PIN_OLED_DATA 21 
  #define CDPCFG_PIN_OLED_RESET 16 
  #define CDPCFG_PIN_OLED_ROTATION U8G2_R0 
  // actualy missing 
  #define CDPCFG_PIN_LORA_DIO1 -1



/*
 * BOARD "heltec wireless stick"
 * http://www.heltec.cn/project/wireless-stick/
 * pio: board = heltec_wireless_stick
 */
#elif defined(ARDUINO_HELTEC_WIRELESS_STICK)

#define CDPCFG_PIN_BAT 37
#define CDPCFG_BAT_MULDIV 320 / 100

#define CDPCFG_PIN_VEXT 21

#define CDPCFG_PIN_LED1 25

// Lora configurations
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_RST 14

// Oled Display settings
#define CDPCFG_OLED_64x32
#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA 4
#define CDPCFG_PIN_OLED_RESET 16

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

/*
 * BOARD "heltec wireless stick lite"
 * ARDUIONO BOARD PACKAGE URL:
 * https://resource.heltec.cn/download/package_heltec_esp32_index.json
 * DOCS:
 * http://www.heltec.cn/project/wireless-stick-lite/
 * pio: board = WIRELESS_STICK_LITE
 */
#elif defined(ARDUINO_WIRELESS_STICK_LITE)

#define CDPCFG_PIN_BAT 37
#define CDPCFG_BAT_MULDIV 320 / 100

#define CDPCFG_PIN_VEXT 21

#define CDPCFG_PIN_LED1 25

// Lora configurations
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_RST 14

// Oled Display settings
#define CDPCFG_OLED_NONE

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

/*
 * BOARD "Heltec Cube Cell Board ASR6501 with SX1262"
 * https://heltec.org/project/htcc-ab01/
 * pio: board = cubecell_board
 */
#elif defined(CubeCell_Board)

#define CDPCFG_HELTEC_CUBE_CELL

// TODO: These are not used by the Heltec LoRa library
// But they still need to be defined for now because the Duck::setupRadio() uses them.
// Update the setupRadio to use a RadioConfig data structure instead so we don't need to expose these to the apps
#define CDPCFG_PIN_LORA_CS 10
#define CDPCFG_PIN_LORA_DIO0 9 // BUSY PIN
#define CDPCFG_PIN_LORA_DIO1 8
#define CDPCFG_PIN_LORA_DIO2 -1
#define CDPCFG_PIN_LORA_RST 14

// Oled Display settings
#define CDPCFG_OLED_NONE

// Wifi module
#define CDPCFG_WIFI_NONE
//===== BOARD "Heltec Cube Cell Board ASR6501 with SX1262" =====

#elif defined(CubeCell_GPS)

#define CDPCFG_HELTEC_CUBE_CELL

// Uncomment this to enable the OLED display
//#define ENABLE_DISPLAY

// TODO: These are not used by the Heltec LoRa library
// But they still need to be defined for now because the Duck::setupRadio() uses
// them. Update the setupRadio to use a RadioConfig data structure instead so we
// don't need to expose these to the apps
#define CDPCFG_PIN_LORA_CS 10
#define CDPCFG_PIN_LORA_DIO0 9 // BUSY PIN
#define CDPCFG_PIN_LORA_DIO1 8
#define CDPCFG_PIN_LORA_DIO2 -1
#define CDPCFG_PIN_LORA_RST 14

// Oled Display settings
#ifdef ENABLE_DISPLAY
#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA 4
#define CDPCFG_PIN_OLED_RESET 16
#else
#define CDPCFG_OLED_NONE
#endif

// Wifi module
#define CDPCFG_WIFI_NONE
//===== BOARD "Heltec Cube Cell Board ASR6502+GPS+DISPLAY with SX1262" =====

/*
 * BOARD "rocket scream Mini Ultra Pro v3"
 * https://www.rocketscream.com/blog/docs-item/mini-ultra-pro-hookup-guide/
 * pio: board = arduino_zero
 */
#elif defined(ARDUINO_SAMD_ZERO)
// This is not an official ARDUINO_* but uses the same bootloader
// as the SAMD21 based Arduino Zero
//
// Lora configurations (HopeRF RFM95W chip)
#define CDPCFG_PIN_LORA_CS 5
#define CDPCFG_PIN_LORA_DIO0 2
#define CDPCFG_PIN_LORA_DIO1 6
#define CDPCFG_PIN_LORA_DIO2 -1 // Unused
#define CDPCFG_PIN_LORA_RST 3

// Oled Display settings
#define CDPCFG_OLED_NONE

// Wifi module
#define CDPCFG_WIFI_NONE

// Required for Serial on Zero based boards
#define Serial SERIAL_PORT_USBVIRTUAL
//===== BOARD "rocket scream Mini Ultra Pro v3" =====

/*
 * BOARD "sparkfun lora gateway 1-channel"
 * https://www.sparkfun.com/products/15006
 * pio: board = sparkfun_lora_gateway_1-channel
 */
#elif defined(SPARKFUN_LGW1C)
// this is not an official ARDUINO_* define since this board doesnt have one

// Lora configurations
#define CDPCFG_PIN_LORA_CS 16
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_DIO1 33
#define CDPCFG_PIN_LORA_DIO2 32
#define CDPCFG_PIN_LORA_RST 5

// Oled Display settings
#define CDPCFG_OLED_NONE

/*
 * BOARD "pycom lopy"
 * https://docs.pycom.io/datasheets/development/lopy/
 * pio: board = lopy
 */
#elif defined(ARDUINO_LoPy)

#define CDPCFG_PIN_ANT 16

// Lora configurations
#define CDPCFG_PIN_LORA_CS 17
#define CDPCFG_PIN_LORA_DIO0 23
#define CDPCFG_PIN_LORA_RST 18

// special SPI
#define CDPCFG_PIN_LORA_SPI_SCK 5
#define CDPCFG_PIN_LORA_SPI_MISO 19
#define CDPCFG_PIN_LORA_SPI_MOSI 27
#define CDPCFG_LORA_CLASS SX1272

// Oled Display settings
#define CDPCFG_OLED_NONE

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

/*
 * BOARD "pycom lopy4"
 * https://docs.pycom.io/datasheets/development/lopy4/
 * pio: board = lopy4
 */
#elif defined(ARDUINO_LoPy4)

#define CDPCFG_PIN_ANT 16

// Lora configurations
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 23
#define CDPCFG_PIN_LORA_RST -1

// special SPI
#define CDPCFG_PIN_LORA_SPI_SCK 5
#define CDPCFG_PIN_LORA_SPI_MISO 19
#define CDPCFG_PIN_LORA_SPI_MOSI 27
#define CDPCFG_LORA_CLASS SX1276

// Oled Display settings
#define CDPCFG_OLED_NONE

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

#else // Default to WIFI_LORA_32_V2 board

#if !defined(ARDUINO_HELTEC_WIFI_LORA_32_V2)
#warning "NO BOARD DEFINED, DEFAULTING TO HELTEC v2"
#define CDPCFG_BOARD_DEFAULT
#endif

/*
 * BOARD "heltec lora v2"
 * https://heltec.org/project/wifi-lora-32/
 * pio: board = heltec_wifi_lora_32_V2
 */

#define CDPCFG_PIN_BAT 37
#define CDPCFG_BAT_MULDIV 320 / 100

#define CDPCFG_PIN_VEXT 21

#define CDPCFG_PIN_LED1 25

// Lora configurations
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_RST 14

// Oled Display settings
#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA 4
#define CDPCFG_PIN_OLED_RESET 16
#define CDPCFG_PIN_OLED_ROTATION U8G2_R0

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

#endif  // Board definitions

/**
 * @brief Non board specific configuration
 *
 */

// Username and Password for OTA web page
#define CDPCFG_UPDATE_USERNAME "username"
#define CDPCFG_UPDATE_PASSWORD "password"

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
#endif
#endif

#endif // CDPCFG

// append optional post-cfg
#ifndef CRPCFG_POST
#define CRPCFG_POST
#if __has_include("cdpcfg-post.h")
#include "cdpcfg-post.h"
#endif
#endif
