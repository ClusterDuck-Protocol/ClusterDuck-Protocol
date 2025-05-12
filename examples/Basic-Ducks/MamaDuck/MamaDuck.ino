/**
 * @file MamaDuck.ino
 * @brief Implements a MamaDuck using the ClusterDuck Protocol (CDP).
 * 
 * This example firmware periodically sends sensor health data (counter and free memory) 
 * through a CDP mesh network. It also relays messages that it receives from other ducks
 * that has not seen yet.
 * 
 * @date 2025-05-07
 */

#include <string>
#include <vector>
#include <arduino-timer.h>
#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// --- Function Declarations ---
bool sendData(std::string message, byte topic=topics::status);
bool runSensor(void *);

// --- Global Variables ---
MamaDuck duck;                        // CDP MamaDuck instance
auto timer = timer_create_default();  // Creating a timer with default settings
const int INTERVAL_MS = 10000;        // Interval in milliseconds between runSensor call
int counter = 1;                      // Counter for the sensor data  
bool setupOK = false;                 // Flag to check if setup is complete

/**
 * @brief Setup function to initialize the MamaDuck
 *
 * - Sets up the Duck device ID (exactly 8 bytes).
 * - Initializes MamaDuck using default configuration.
 * - Sets up periodic execution of sensor data transmissions.
 */
void setup() {

  std::string deviceId("MAMA0001"); // MUST be 8 bytes and unique from other ducks
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[MAMA] Failed to setup MamaDuck");
    return;
  }

  timer.every(INTERVAL_MS, runSensor); // Triggers runSensor every INTERVAL_MS
  
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
    counter++;
    Serial.println("[MAMA] runSensor ok.");
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
    std::string errMessage = "[MAMA] Failed to send data. error = " + std::to_string(err);
    Serial.println(errMessage.c_str());
  }
  return sentOk;
}