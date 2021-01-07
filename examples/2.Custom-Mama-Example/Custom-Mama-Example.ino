#include "timer.h"
#include <MamaDuck.h>
#include <DuckDisplay.h>


auto timer = timer_create_default();
const int INTERVAL_MS = 50000;
char message[32]; 
int counter = 1;

const char* DUCK_WIFI_AP = "MAMA DUCK PORTAL";
// create an instance of a MamaDuck with a given unique id

String deviceName = "MAMA001";

MamaDuck duck = MamaDuck(deviceName);
DuckDisplay* display = NULL;


// LORA RF CONFIG
#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14


bool sendData(const byte* buffer, int length) {
  bool sentOk = false;

  // Send Data can either take a byte buffer (unsigned char) or a vector
  int err = duck.sendData(buffer, length);
  if (err == DUCK_ERR_NONE) {
    counter++;
    sentOk = true;
  }
  if (!sentOk) {
    Serial.println("[MAMA] Failed to send data. error = " + String(err));
  }
  return sentOk;
}


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
//  duck.setupWebServer(true, HTML);
  // initialize Over The Air firmware upgrade
  duck.setupOTA();
  // This duck has an OLED display and we want to use it. 
  // Get an instance and initialize it, so we can use in our application
  display = DuckDisplay::getInstance();
  display->setupDisplay(duck.getType(), deviceName);
  // we are done
  display->showDefaultScreen();

  timer.every(INTERVAL_MS, runSensor);
}

void loop(){
      timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();
};

bool runSensor(void*) {
  bool result;
  const byte* buffer;

  String message = String("mama0001:") + String(counter);
  int length = message.length();
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message);
  buffer = (byte*)message.c_str();

  result = sendData(buffer, length);
  if (result) {
    Serial.println("[MAMA] runSensor ok.");
  } else {
    Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

