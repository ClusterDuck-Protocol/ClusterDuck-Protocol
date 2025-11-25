/**
 * @file DuckLink.ino
 * @brief Implements a DuckLink using the ClusterDuck Protocol (CDP).
 * 
 * This example firmware periodically sends sensor health data (counter and free memory) 
 * through a CDP mesh network.
 * 
 * @date 2025-05-07
 */

 #include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// --- Function Declarations ---
bool runSensor(void *);
bool sendData(std::string message, byte topic=topics::status);

// --- Global Variables ---
DuckLink duck("Link1276");                            // CDP DuckLink instance
auto timern = timer_create_default();      // Creating a timer with default settings
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
  if (duck.setupWithDefaults() != DUCK_ERR_NONE) {
    Serial.println("[LINK] Failed to setup DuckLink");
    return;
  }

         // Triggers runSensor every INTERVAL_MS
  
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

  duckutils::Timer(INTERVAL_MS,runSensor, nullptr);
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
  bool failure;
  
 //  std::string message = "C:" + std::to_string(counter) + "|" + "FM:" + std::to_string(freeMemory());
 //  Serial.print("[DUCKLINK] sensor data: ");
 //  Serial.println(message.c_str());

 //  failure = duck.sendData(topics::health, message);
 //  if (!failure) {
 //    counter++;
 //    Serial.println("[DUCKLINK] runSensor ok.");
 //  } else {
 //    Serial.println("[DUCKLINK] runSensor failed.");
 //  }
  return true;
}
