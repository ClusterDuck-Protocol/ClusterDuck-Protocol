#include "timer.h"
#include <MamaDuck.h>
#include <DuckDisplay.h>



const char* DUCK_WIFI_AP = "MAMA DUCK PORTAL";
// create an instance of a MamaDuck with a given unique id
MamaDuck duck = MamaDuck("DUCK-UID");
DuckDisplay* display = NULL;
// LORA RF CONFIG
#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14
void setup() {
  // initialize the serial component with the hardware supported baudrate
  duck.setupSerial(115200);
  // initialize the LoRa radio with specific settings. This will overwrites settings defined in the CDP config file cdpcfg.h
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_DIO1_PIN, LORA_TXPOWER);
  // initialize the wifi access point with a custom AP name
  duck.setupWifi(DUCK_WIFI_AP);
  // initialize DNS
  duck.setupDns();
  // initialize web server, enabling the captive portal with a custom HTML page
  duck.setupWebServer(true, HTML);
  // initialize Over The Air firmware upgrade
  duck.setupOTA();
  // This duck has an OLED display and we want to use it. 
  // Get an instance and initialize it, so we can use in our application
  display = DuckDisplay::getInstance();
  display->setupDisplay();
  // we are done
  display->drawString(true, 20,20, "DUCK READY");
  Serial.println("MAMA-DUCK...READY!");
  timer.every(INTERVAL_MS, runSensor);
}
