/**
 * @file Custom-Mama-Example.ino
 * @brief Implements a MamaDuck using the CDP APIs.
 * 
 * Configures a MamaDuck with full access to the CDP APIs, including LoRa radio setup, optional 
 * WiFi + captive portal, and OLED display output. It periodically transmits system health 
 * data (counter + free memory) over the CDP mesh, and relays unseen messages from other ducks.
 * 
 * @date 2025-05-09
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// LoRa configuration for US915 MHz
#define LORA_FREQ     915.0
#define LORA_TXPOWER  20
#define LORA_CS_PIN   18
#define LORA_DIO0_PIN 26
#define LORA_DIO1_PIN -1
#define LORA_RST_PIN  14

// --- Global Variables ---
MamaDuck duck;                          // CDP MamaDuck instance
DuckDisplay* display = NULL;            // OLED display instance
auto timer = timer_create_default();    // Main loop timer for telemetry
const int INTERVAL_MS = 10000;          // Interval between health messages
int counter = 1;                        // Counter sent as part of health telemetry
bool setupOK = false;                   // Tracks successful setup

// --- Function Declarations ---
bool sendData(std::string message, byte topic=topics::status);
bool runSensor(void *);

/**
 * @brief Setup function to initialize the MamaDuck
 *
 * - Sets up the Duck device ID (exactly 8 bytes).
 * - Initializes MamaDuck using MamaDuck APIs.
 * - Initializes the OLED display.
 * - Sets up periodic execution of sensor data transmissions.
 */
void setup() {

  std::string deviceId("MAMA0001");       // MUST be 8 bytes and unique from other ducks
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());

  // Set the Device ID
  duck.setDeviceId(deviceId);
  
  // Initialize the serial component with the hardware supported baudrate
  duck.setupSerial(115200);
  
  // Initialize the LoRa radio with specific settings. Overwrites settings defined in the CDP config file cdpcfg.h
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

  timer.every(INTERVAL_MS, runSensor);  // Triggers runSensor every INTERVAL_MS
  
  setupOK = true;
  Serial.println("[MAMA] Setup OK!");

}

/**
 * @brief Main loop runs continuously.
 *
 * Executes scheduled tasks and maintains Duck operation.
 */
void loop() {
  if (!setupOK) {
    return; 
  }
  timer.tick();

  duck.run();
}

/**
 * @brief Periodically executed to gather and send health data.
 *
 * Collects the current counter value and available free memory, formats them
 * into a string, and transmits this data via CDP.
 *
 * @param unused Unused parameter required by the timer callback signature
 * @return true if data was successfully sent, false otherwise
 */
bool runSensor(void *) {
  
  bool result;
  
  std::string message = "C:" + std::to_string(counter) + "|" + "FM:" + std::to_string(freeMemory());
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message.c_str());

  result = sendData(message, topics::health);
  if (result) {
     Serial.println("[MAMA] runSensor ok.");
     counter++;
  } else {
     Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

/**
 * @brief Sends the provided message as CDP packet to the mesh network.
 *
 * Encapsulates the message within a CDP topic and handles errors in transmission. 
 *
 * @param message The payload data to send as a std::string
 * @param topic CDP topic. CDP topics can be found in CdpPacket.h (default: status)
 * @return true if data sent successfully, false otherwise
 */
bool sendData(std::string message, byte topic) {
  bool sentOk = false;
  
  int err = duck.sendData(topic, message);
  if (err == DUCK_ERR_NONE) {
     sentOk = true;
  }
  if (!sentOk) {
    Serial.println(("[MAMA] Failed to send data. error = " + std::to_string(err)).c_str());
  }
  return sentOk;
}