
/**
 * @file Custom-Mama-Example.ino
 * @brief Uses the built in Mama Duck.
 * 
 * This example is a Custom Mama Duck with an Oled display, and it is
 * periodically sending a message in the Mesh
 * 
 */

#include <string>
#include <arduino-timer.h>
#include <MamaDuck.h>

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
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());


  // Set the Device ID
  duck.setDeviceId(deviceId);
  // Initialize the serial component with the hardware supported baudrate
  duck.setupSerial(115200);
  // Initialize the LoRa radio with specific settings. This will overwrites settings defined in the CDP config file cdpcfg.h
  duck.setupRadio(LORA_FREQ, LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN, LORA_DIO1_PIN, LORA_TXPOWER);
  // Initialize the wifi access point with a custom AP name
  duck.setupWifi();
  // Initialize DNS
  duck.setupDns();
  // Initialize web server, enabling the captive portal with a custom HTML page
  duck.setupWebServer(true);
  // Get a display instance and initialize it, so we can use in our application
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
  
  std::string message = "C:" + std::to_string(counter) + "|" + "FM:" + std::to_string(freeMemory());
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message.c_str());

  result = sendData(stringToByteVector(message));
  if (result) {
     Serial.println("[MAMA] runSensor ok.");
  } else {
     Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

bool sendData(std::vector<byte> message) {
  bool sentOk = false;
  
  int err = duck.sendData(topics::status, message);
  if (err == DUCK_ERR_NONE) {
     counter++;
     sentOk = true;
  }
  if (!sentOk) {
    Serial.println(("[MAMA] Failed to send data. error = " + std::to_string(err)).c_str());
  }
  return sentOk;
}