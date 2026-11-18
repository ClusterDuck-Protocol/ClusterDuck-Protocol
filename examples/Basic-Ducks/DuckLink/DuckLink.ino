/**
 * @file DuckLink.ino
 * @brief A minimal DuckLink for the ClusterDuck Protocol (CDP) 5.5.
 *
 * A DuckLink is a leaf node: it sends its own messages into the mesh but does
 * not relay messages from other ducks. This sketch sends a short text message
 * on topics::sensor every SEND_INTERVAL_MS. MamaDucks in range relay it to the
 * PapaDuck.
 *
 * - There is no network to join: the duck can send as soon as
 *   setupWithDefaults() has started the radio.
 * - sendData() returns DUCK_ERR_NONE once the message is queued for the radio.
 *   That is not a delivery receipt.
 * - CDP sends this duck's own health on topics::health every 15 minutes. Send
 *   your own data on another topic, such as topics::sensor.
 */

#include <string>
#include <CDP.h>

// --- Configuration ---
const char DEVICE_ID[] = "DUCKLINK";          // exactly 8 characters, unique in the mesh
const unsigned long SEND_INTERVAL_MS = 10000;  // time between two messages

// --- Globals ---
DuckLink duck(DEVICE_ID);
auto timer = timer_create_default();
uint32_t counter = 1;   // number of the next message
bool setupOK = false;

bool sendMessage(void*);

void setup() {
  // Starts the serial port at 115200 baud and the LoRa radio.
  int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[LINK] Setup failed, rc = %d (%s)\n", err, getDuckErrorString(err));
    return;
  }

  timer.every(SEND_INTERVAL_MS, sendMessage);
  setupOK = true;
  Serial.println("[LINK] Setup OK");
}

void loop() {
  if (!setupOK) {
    return;
  }
  timer.tick();
  duck.run();   // sends queued messages and handles received packets
}

/**
 * @brief Queue one message for the mesh. Called by the timer.
 * @returns true to keep the timer running
 */
bool sendMessage(void*) {
  std::string message = "Hello from " + std::string(DEVICE_ID) + " C:" + std::to_string(counter);

  int err = duck.sendData(topics::sensor, message);
  if (err == DUCK_ERR_NONE) {
    Serial.printf("[LINK] Queued: %s\n", message.c_str());
    counter++;
  } else {
    Serial.printf("[LINK] sendData failed, rc = %d (%s)\n", err, getDuckErrorString(err));
  }
  return true;
}
