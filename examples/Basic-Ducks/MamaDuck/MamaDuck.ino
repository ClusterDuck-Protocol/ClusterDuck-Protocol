/**
 * @file MamaDuck.ino
 * @brief A minimal MamaDuck for the ClusterDuck Protocol (CDP) 5.5.
 *
 * A MamaDuck is the core node of the mesh: it relays the messages it hears
 * from other ducks (each one once, for up to 6 hops) and can send its own.
 * This sketch sends a short text message on topics::sensor every
 * SEND_INTERVAL_MS. Relaying needs no code: duck.run() does it.
 *
 * - There is no network to join: the duck can send as soon as
 *   setupWithDefaults() has started the radio.
 * - sendData() returns DUCK_ERR_NONE once the message is queued for the radio.
 *   That is not a delivery receipt.
 * - CDP sends this duck's own health on topics::health every 15 minutes. Send
 *   your own data on another topic, such as topics::sensor.
 *
 * For a MamaDuck with a status LED, a JSON payload and the optional 5.5
 * features, see examples/Custom-Mama-Examples/Full-MamaDuck.
 */

#include <string>
#include <CDP.h>

// --- Configuration ---
const char DEVICE_ID[] = "MAMADUCK";          // exactly 8 characters, unique in the mesh
const unsigned long SEND_INTERVAL_MS = 10000;  // time between two messages

// --- Globals ---
MamaDuck duck(DEVICE_ID);
auto timer = timer_create_default();
uint32_t counter = 1;   // number of the next message
bool setupOK = false;

bool sendMessage(void*);

void setup() {
  // Starts the serial port at 115200 baud and the LoRa radio.
  int err = duck.setupWithDefaults();
  if (err != DUCK_ERR_NONE) {
    Serial.printf("[MAMA] Setup failed, rc = %d (%s)\n", err, getDuckErrorString(err));
    return;
  }

  timer.every(SEND_INTERVAL_MS, sendMessage);
  setupOK = true;
  Serial.println("[MAMA] Setup OK");
}

void loop() {
  if (!setupOK) {
    return;
  }
  timer.tick();
  duck.run();   // relays received packets and sends queued messages
}

/**
 * @brief Queue one message for the mesh. Called by the timer.
 * @returns true to keep the timer running
 */
bool sendMessage(void*) {
  std::string message = "Hello from " + std::string(DEVICE_ID) + " C:" + std::to_string(counter);

  int err = duck.sendData(topics::sensor, message);
  if (err == DUCK_ERR_NONE) {
    Serial.printf("[MAMA] Queued: %s\n", message.c_str());
    counter++;
  } else {
    Serial.printf("[MAMA] sendData failed, rc = %d (%s)\n", err, getDuckErrorString(err));
  }
  return true;
}
