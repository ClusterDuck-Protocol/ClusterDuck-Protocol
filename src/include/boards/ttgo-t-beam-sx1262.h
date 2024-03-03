// CDP 
#define CDPCFG_RADIO_SX126X

#define CDPCFG_PIN_BAT 35 
#define CDPCFG_BAT_MULDIV 200 / 100 
#define CDPCFG_PIN_LED1 25

// LoRa configuration
#define RADIO_CS_PIN                18
#define RADIO_DI0_PIN               26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define CDPCFG_LORA_CLASS SX1262             
#define CDPCFG_PIN_LORA_CS RADIO_CS_PIN
#define CDPCFG_PIN_LORA_DIO0 RADIO_DI0_PIN
#define CDPCFG_PIN_LORA_DIO1 RADIO_DIO1_PIN
#define CDPCFG_PIN_LORA_RST RADIO_RST_PIN 
#define CDPCFG_PIN_LORA_BUSY RADIO_BUSY_PIN

// Oled Display settings
#define CDPCFG_OLED_NONE
