// TODO: These are not used by the Heltec LoRa library
// But they still need to be defined for now because the Duck::setupRadio() uses them.
// Update the setupRadio to use a RadioConfig data structure instead so we don't need to expose these to the apps
#define CDPCFG_PIN_LORA_CS 10
#define CDPCFG_PIN_LORA_DIO0 9 // BUSY PIN
#define CDPCFG_PIN_LORA_DIO1 8
#define CDPCFG_PIN_LORA_DIO2 -1
#define CDPCFG_PIN_LORA_RST 14

// OLED display settings
#define CDPCFG_OLED_NONE

// WiFi module
#define CDPCFG_WIFI_NONE
