# Adding Your Own Board
This directory contains board-specific files for the various boards that are supported by the CDP firmware.

## Supported Boards

The ClusterDuck Protocol currently supports the following boards out of the box:

- **Heltec WiFi LoRa 32 V2** - `src/include/boards/heltec_wifi_lora_32_V2.h`
- **Heltec WiFi LoRa 32 V3** - `src/include/boards/heltec_wifi_lora_32_V3.h`
- **LilyGO T-Beam SX1262** - `src/include/boards/lilygo_t_beam_sx1262.h`
- **TTGO T-Beam V1 SX1276** - `src/include/boards/ttgo_t_beam_v1_sx1276.h`

## Board Format
Some boards will differ in capability, so not all pins will need to be defined. All pins starting 
Here is what a board file should look like to start:

```cpp
#define CDPCFG_PIN_BAT 
#define CDPCFG_BAT_MULDIV 

#define CDPCFG_PIN_LED1 

// LoRa configuration
#define CDPCFG_PIN_LORA_CS 
#define CDPCFG_PIN_LORA_DIO0 
#define CDPCFG_PIN_LORA_DIO1 
#define CDPCFG_PIN_LORA_RST 

//GPS configuration
#define CDPCFG_GPS_RX 
#define CDPCFG_GPS_TX 

// OLED display settings
#define CDPCFG_PIN_OLED_CLOCK 
#define CDPCFG_PIN_OLED_DATA 
#define CDPCFG_PIN_OLED_RESET 
#define CDPCFG_PIN_OLED_ROTATION 
```
Any pins currently supported by the firmware directly will need `CDPCFG_` at the beginning of the pin name. If the pin is not supported by the firmware, you can define it as you see fit for your own purposes.
The pins all **LoRa** boards will have in common are:
```
CDPCFG_PIN_LED1 
CDPCFG_PIN_LORA_CS 
CDPCFG_PIN_LORA_DIO0
CDPCFG_PIN_LORA_DIO1
CDPCFG_PIN_LORA_RST 
```
If you are using a **GPS** module, you will also need to define the RX and TX pins:
```
CDPCFG_GPS_RX
CDPCFG_GPS_TX
```
If you are using an **OLED** display, you will also need to define the following pins:
```
CDPCFG_PIN_OLED_CLOCK
CDPCFG_PIN_OLED_DATA
CDPCFG_PIN_OLED_RESET
CDPCFG_PIN_OLED_ROTATION
```
If you are using a board with a **battery** module such as the TTGO T-Beam, you will also need to define the following pins:
```
CDPCFG_PIN_BAT 
CDPCFG_BAT_MULDIV 
```
The GPS, Battery and OLED pins are optional, but if you are using them, you will need to define them in your board file.

## Defining the Pins
The pins for each board are in a pinout diagram made by the manufacturer. The pin numbers you're looking for are the GPIO
numbers, not the physical pin numbers. For example, the TTGO T-Beam has the following pinout diagram:

![TTGO T-Beam Example Pinout](https://www.passion-radio.fr/img/cms/v1-2_1.png)
**NOTE: The manufurers may change the pinout diagram or have the wrong pins labeled, so cross reference with other sources
or verify the pins with the manufacturer.**

After you have the pins you need, they should be defined in the board file like so:
```cpp
#define CDPCFG_PIN_BAT 35
#define CDPCFG_BAT_MULDIV 200 / 100

#define CDPCFG_PIN_LED1 25

// LoRa configuration
#define CDPCFG_PIN_LORA_CS 18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_DIO1 -1
#define CDPCFG_PIN_LORA_RST 14

//GPS configuration
#define CDPCFG_GPS_RX 34
#define CDPCFG_GPS_TX 12

// OLED display settings
#define CDPCFG_PIN_OLED_CLOCK 22
#define CDPCFG_PIN_OLED_DATA 21
#define CDPCFG_PIN_OLED_RESET 16
#define CDPCFG_PIN_OLED_ROTATION U8G2_R0
```

## Adding the Board Header
After you've defined your header file as shown above, you have the option of putting it in the `src/include/boards` directory as `cdp_external_board.h` or somewhere else more convenient with whatever filename you choose. If you place it outside that standard directory, you will need to include it in the `src/include/cdpcfg.h` file
where the other board headers are included:
```cpp
...

#ifdef CDP_EXTERNAL_BOARD
#include "boards/cdp_external_board.h"
#else
#include "boards/heltec_wifi_lora_32_V3.h"
#include "boards/heltec_wifi_lora_32_V2.h"
#include "boards/lilygo_t_beam_sx1262.h"
#include "boards/ttgo_t_beam_v1_sx1276.h"
// Extra boards (available but not officially supported/maintained)
#include "boards/extra_boards/lilygo_t_beam_supreme_sx1262.h"
#include "boards/extra_boards/lilygo_sim7000g_lora.h"
#endif

...
```