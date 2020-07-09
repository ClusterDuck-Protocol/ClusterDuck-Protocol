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
 * - create a from-scratch cdpcfg-pre.h that just overrides f.ex. the board defines or
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

    //Lora configurations
    #define CDPCFG_PIN_LORA_CS   18
    #define CDPCFG_PIN_LORA_DIO0 26
    #define CDPCFG_PIN_LORA_RST  23

    // Oled Display settings
    #define CDPCFG_PIN_OLED_CLOCK 15
    #define CDPCFG_PIN_OLED_DATA   4
    #define CDPCFG_PIN_OLED_RESET 16

    // actualy missing
    #define CDPCFG_PIN_LORA_DIO1  -1



  /*
   * BOARD "ttgo lora v2"
   * top right in this picture
   * https://github.com/Xinyuan-LilyGO/TTGO-LoRa-Series
   * pio: board = ttgo-lora32-v2
   */
  #elif defined(ARDUINO_TTGO_LoRa32_V2)
    #define CDPCFG_PIN_BAT 35
    #define CDPCFG_BAT_MULDIV 200/100

    #define CDPCFG_PIN_LED1 25

    //Lora configurations
    #define CDPCFG_PIN_LORA_CS   18
    #define CDPCFG_PIN_LORA_DIO0 26
    #define CDPCFG_PIN_LORA_RST  14

    // Oled Display settings
    #define CDPCFG_PIN_OLED_CLOCK 22
    #define CDPCFG_PIN_OLED_DATA  21

    // actualy missing
    #define CDPCFG_PIN_OLED_RESET -1
    #define CDPCFG_PIN_LORA_DIO1  -1



  /*
   * BOARD "heltec lora v2"
   * https://heltec.org/project/wifi-lora-32/
   * pio: board = heltec_wifi_lora_32_V2
   */
  #elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V2)

    #define CDPCFG_PIN_BAT  37
    #define CDPCFG_BAT_MULDIV 320/100

    #define CDPCFG_PIN_VEXT 21

    #define CDPCFG_PIN_LED1 25

    //Lora configurations
    #define CDPCFG_PIN_LORA_CS   18
    #define CDPCFG_PIN_LORA_DIO0 26
    #define CDPCFG_PIN_LORA_RST  14

    // Oled Display settings
    #define CDPCFG_PIN_OLED_CLOCK 15
    #define CDPCFG_PIN_OLED_DATA   4
    #define CDPCFG_PIN_OLED_RESET 16

    // actualy missing
    #define CDPCFG_PIN_LORA_DIO1 -1

  #else
    #error "no known board defined"
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
  #define CDPCFG_RF_LORA_FREQ  915.0
  #define CDPCFG_RF_LORA_BW    125.0
  #define CDPCFG_RF_LORA_SF      7
  #define CDPCFG_RF_LORA_TXPOW  20
  #define CDPCFG_RF_LORA_GAIN    0

  // cdp configuration
  #define CDPCFG_CDP_BUFSIZE 250
  #define CDPCFG_UUID_LEN 8

  // Timer in milliseconds
  #define CDPCFG_MILLIS_ALIVE   1800000
  #define CDPCFG_MILLIS_REBOOT 43200000


  // RGB Led Pins
  #define CDPCFG_PIN_RGBLED_R 25
  #define CDPCFG_PIN_RGBLED_G  4
  #define CDPCFG_PIN_RGBLED_B  2


#endif // CDPCFG


// append optional post-cfg
#ifndef CRPCFG_POST
  #define CRPCFG_POST
  #if __has_include("cdpcfg-post.h")
    #include "cdpcfg-post.h"
  #endif
#endif



