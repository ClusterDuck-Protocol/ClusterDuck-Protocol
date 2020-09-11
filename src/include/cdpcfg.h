/*
 * CDP central compile-time configuration
 *
 * it will include optional
 * - cdpcfg-pre.h at the beginning
 * - cdpcfg-post.h at the end
 *
 * to customize your build, you could ...
 * - edit this file or
 * - copy it to cdpcfg-pre.h and edit _that_ or
 * - create a from-scratch cdpcfg-pre.h that just overrides f.ex. the board
 * defines or
 * - create a cdpcfg-post.h to undef/define just parts
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
#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA 4
#define CDPCFG_PIN_OLED_RESET 16

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

/*
 * BOARD "ttgo lora v2"
 * top right in this picture
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
#define CDPCFG_PIN_OLED_CLOCK 22
#define CDPCFG_PIN_OLED_DATA 21

// actualy missing
#define CDPCFG_PIN_OLED_RESET -1
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

#else

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

// actualy missing
#define CDPCFG_PIN_LORA_DIO1 -1

#endif

// non BOARD-specific config

// Username and Password for OTA web page
#define CDPCFG_UPDATE_USERNAME "username"
#define CDPCFG_UPDATE_PASSWORD "password"

// Serial Console Baud Rate
#define CDPCFG_SERIAL_BAUD 115200

// Access point IP adress
#define CDPCFG_AP_IP1 192
#define CDPCFG_AP_IP2 168
#define CDPCFG_AP_IP3 1
#define CDPCFG_AP_IP4 1

// Asyncwebserver Port
#define CDPCFG_WEB_PORT 80

// Lora RF configuration
#define CDPCFG_RF_LORA_FREQ 915.0
#define CDPCFG_RF_LORA_BW 125.0
#define CDPCFG_RF_LORA_SF 7
#define CDPCFG_RF_LORA_TXPOW 20
#define CDPCFG_RF_LORA_GAIN 0

// cdp configuration
#define CDPCFG_CDP_BUFSIZE 256
#define CDPCFG_UUID_LEN 8

// Timer in milliseconds
#define CDPCFG_MILLIS_ALIVE 1800000
#define CDPCFG_MILLIS_REBOOT 43200000

// RGB Led Pins
#define CDPCFG_PIN_RGBLED_R 25
#define CDPCFG_PIN_RGBLED_G 4
#define CDPCFG_PIN_RGBLED_B 2

// semiautomatic section for setting defaults

// set default modem
#ifndef CDPCFG_LORA_CLASS
#define CDPCFG_LORA_CLASS SX1276
#endif

// set oled class
#ifndef CDPCFG_OLED_CLASS
#if defined(CDPCFG_OLED_NONE)
// do nothing
#elif defined(CDPCFG_OLED_64x32)
// smaller displays
#define CDPCFG_OLED_CLASS U8X8_SSD1306_64X32_NONAME_SW_I2C
#else
// classic default
#define CDPCFG_OLED_CLASS U8X8_SSD1306_128X64_NONAME_SW_I2C
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
