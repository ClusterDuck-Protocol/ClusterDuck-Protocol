/**
 * @file DuckLink.ino
 * @brief Implements a DuckLink using the ClusterDuck Protocol (CDP).
 * 
 * This example firmware periodically sends sensor health data (counter and free memory) 
 * through a CDP mesh network.
 * 
 * @date 2025-05-07
 */

#include <arduino-timer.h>
#include <string>
#include <vector>
#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// --- Function Declarations ---
bool runSensor(void *);
bool sendData(std::string message, byte topic=topics::status);

// --- Global Variables ---
DuckLink duck;                            // CDP DuckLink instance
auto timer = timer_create_default();      // Creating a timer with default settings
const int INTERVAL_MS = 10000;            // Interval in milliseconds between each call of runSensor
int counter = 1;                          // Message counter to track transmissions
bool setupOK = false;                     // Flag indicating setup completion status

/**
 * @brief Setup function initializing the DuckLink.
 *
 * - Sets up the Duck ID (exactly 8 bytes).
 * - Initializes DuckLink using default configuration.
 * - Sets up periodic execution of sensor data transmissions.
 */
void setup() {

  std::string deviceId("DUCK0001");    // MUST be 8 bytes and unique from other ducks
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[LINK] Failed to setup DuckLink");
    return;
  }
  
  timer.every(INTERVAL_MS, runSensor);     // Triggers runSensor every INTERVAL_MS
  
  Serial.println("[LINK] Setup OK!");
  setupOK = true;
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
 * @brief Function to run the sensor and send data.
 *
 * This function is called periodically to send sensor health data.
 *
 * @param unused Pointer to unused parameter (not used in this implementation)
 * @return true if data was sent successfully, false otherwise
 */
bool runSensor(void *) {
  
  bool result = false;

  std::string message = "C:" + std::to_string(counter) + "|" + "FM:" + std::to_string(freeMemory());
  Serial.print("[LINK] sensor data: ");
  Serial.println(message.c_str());
  
  result = sendData(message, topics::health);
  if (result) {
     Serial.println("[LINK] runSensor ok.");
  } else {
     Serial.println("[LINK] runSensor failed.");
  }
  return result;

}

/**
 * @brief Function to send data through the DuckLink.
 *
 * Encapsulates the message within a CDP topic and handles errors in transmission. CDP topics can be found 
 * in CdpPacket.h
 *
 * @param message The message to send
 * @param topic The topic to send the message to (default is status)
 * @return true if data was sent successfully, false otherwise
 */
bool sendData(std::string message, byte topic) {
  bool sentOk = false;
  
  int err = duck.sendData(topic, message);
  if (err == DUCK_ERR_NONE) {
    sentOk = true;
  }
  if (!sentOk) {
    std::string errMessage = "[Link] Failed to send data. error = " + std::to_string(err);
    Serial.println(errMessage.c_str());
  }
  return sentOk;
}