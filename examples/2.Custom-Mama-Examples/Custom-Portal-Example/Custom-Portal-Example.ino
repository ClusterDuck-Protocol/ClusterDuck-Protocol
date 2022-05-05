
/**
 * @file Custom-Portal-Example.ino
 * @brief Uses the built in Mama Duck.
 * 
 * This example is a Custom Mama Duck with a captive emergency portal, and it is periodically sending a message in the Mesh
 * 
 */

#include <string>
#include <arduino-timer.h>
#include <MamaDuck.h>
// You can build and modify your own Captive Portal by making edits to emergencyPortal.h 
#include "emergencyPortal.h"

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in mama duck
MamaDuck duck;

// Create a Display Instance
DuckDisplay* display = NULL;

// LORA RF CONFIG
#define LORA_FREQ 915.0 // Frequency Range. Set for US Region 915.0Mhz
#define LORA_TXPOWER 20 // Transmit Power
// LORA HELTEC PIN CONFIG
#define LORA_CS_PIN 18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1 // unused
#define LORA_RST_PIN 14


// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 60000;
int counter = 1;

void setup() {
  // The Device ID must be unique and 8 bytes long. Typically the ID is stored
  // in a secure nvram, or provided to the duck during provisioning/registration
  std::string deviceId("MAMA0001");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end()); 


  //Set the Device ID
  duck.setDeviceId(devId);
  // initialize the serial component with the hardware supported baudrate
  duck.setupSerial(115200);
  // initialize the LoRa radio with specific settings. This will overwrites settings defined in the CDP config file cdpcfg.h
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_DIO1_PIN, LORA_TXPOWER);
  // initialize the wifi access point with a custom AP name
  duck.setupWifi();
  // initialize DNS
  duck.setupDns();
  // initialize web server, enabling the captive portal and pass in a Custom Captive portal from emergencyPortal.h
  duck.setupWebServer(true, emergencyPortal);
  // initialize Over The Air firmware upgrade
  duck.setupOTA();

  // This duck has an OLED display and we want to use it. 
  // Get an instance and initialize it, so we can use in our application
  display = DuckDisplay::getInstance();
  display->setupDisplay(duck.getType(), devId);
  display->showDefaultScreen();

  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);
  Serial.println("[MAMA] Setup OK!");

}

void loop() {
  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();
}

bool runSensor(void *) {
  bool result;
  const byte* buffer;
  
  String message = String("Counter:") + String(counter);
  int length = message.length();
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message);
  buffer = (byte*) message.c_str(); 

  result = sendData(buffer, length);
  if (result) {
    Serial.println("[MAMA] runSensor ok.");
  } else {
    Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

bool sendData(const byte* buffer, int length) {
  bool sentOk = false;
  
  // Send Data can either take a byte buffer (unsigned char) or a vector
  int err = duck.sendData(topics::status, buffer, length);
  if (err == DUCK_ERR_NONE) {
    counter++;
    sentOk = true;
  }
  if (!sentOk) {
    Serial.println("[MAMA] Failed to send data. error = " + String(err));
  }
  return sentOk;
}












