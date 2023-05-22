
/*
 * BOARD "WIFI_LoRa_32_V3"
 * https://heltec.org/project/wifi-lora-32-v3/
 */
#define CDPCFG_BOARD WiFi_Lora_32_V3
#define CDPCFG_RADIO_SX126X

// Lora configurations 
#define CDPCFG_LORA_CLASS SX1262             

#define RADIO_CS_PIN                8
#define RADIO_SCLK_PIN              9
#define RADIO_MOSI_PIN              10
#define RADIO_MISO_PIN              11
#define RADIO_RST_PIN               12
#define RADIO_BUSY_PIN              13  // SX1262 BUSY
#define RADIO_DIO1_PIN              14  // SX1262 IRQ

#define CDPCFG_PIN_LORA_CS RADIO_CS_PIN
#define CDPCFG_PIN_LORA_RST RADIO_RST_PIN
#define CDPCFG_PIN_LORA_DIO0 RADIO_BUSY_PIN
#define CDPCFG_PIN_LORA_DIO1 RADIO_DIO1_PIN
#define CDPCFG_PIN_LORA_BUSY RADIO_BUSY_PIN

#define CDPCFG_PIN_LORA_MOSI RADIO_MOSI_PIN
#define CDPCFG_PIN_LORA_SCLK RADIO_SCLK_PIN
#define CDPCFG_PIN_LORA_MISO RADIO_MISO_PIN

// Oled Display settings
#define CDPCFG_PIN_OLED_CLOCK SCL_OLED
#define CDPCFG_PIN_OLED_DATA SDA_OLED
#define CDPCFG_PIN_OLED_RESET RST_OLED
#define CDPCFG_PIN_OLED_ROTATION U8G2_R0