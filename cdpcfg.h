#ifndef CDPCFG
#define CDPCFG

// Username and Password for OTA web page
#define CDPCFG_UPDATE_USERNAME "username"
#define CDPCFG_UPDATE_PASSWORD "password"


// Baud Rate
#define CDPCFG_SERIAL_BAUD 115200

// Oled Display settings
#define CDPCFG_PIN_OLED_CLOCK 15
#define CDPCFG_PIN_OLED_DATA   4
#define CDPCFG_PIN_OLED_RESET 16


// Acces point IP adress
#define CDPCFG_AP_IP1 192
#define CDPCFG_AP_IP2 168
#define CDPCFG_AP_IP3 1
#define CDPCFG_AP_IP4 1


// Asyncwebserver Port
#define CDPCFG_WEB_PORT 80


//Lora configurations
#define CDPCFG_PIN_LORA_CS   18
#define CDPCFG_PIN_LORA_DIO0 26
#define CDPCFG_PIN_LORA_DIO1 25
#define CDPCFG_PIN_LORA_RST  14


#define CDPCFG_RF_LORA_FREQ  915.0
#define CDPCFG_RF_LORA_BW    125.0
#define CDPCFG_RF_LORA_SF      7
#define CDPCFG_RF_LORA_TXPOW  20
#define CDPCFG_RF_LORA_GAIN    0


#define CDPCFG_PIN_LED1 25


#define CDPCFG_CDP_BUFSIZE 250

//Timer in milliseconds
#define CDPCFG_MILLIS_ALIVE   1800000
#define CDPCFG_MILLIS_REBOOT 43200000

//length of the uuid
#define CDPCFG_UUID_LEN 8


//Led Pins
#define CDPCFG_PIN_RGBLED_R 25
#define CDPCFG_PIN_RGBLED_G  4
#define CDPCFG_PIN_RGBLED_B  2



#endif // CDPCFG
