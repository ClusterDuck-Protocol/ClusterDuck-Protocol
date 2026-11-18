/**
 * @file DuckLink.ino
 * @brief A DuckLink for a board that CDP doesn't define itself.
 *
 * The board's pins come from my-boards/<board>/board_config.h. When the build
 * defines CDP_EXTERNAL_BOARD, CDP includes cdp_external_board.h from this
 * folder instead of its own board list, and that file picks the board_config.h
 * for the board the build names (see the .ini file in each my-boards folder).
 * The sketch itself is a plain DuckLink that starts and keeps running.
 */

#include <CDP.h>

DuckLink duck("LINK0001");   // exactly 8 characters, unique in the mesh
bool setupOK = false;

void setup() {
  // Starts the serial port at 115200 baud and the LoRa radio.
  int rc = duck.setupWithDefaults();
  if (rc != DUCK_ERR_NONE) {
    Serial.printf("[LINK] Setup failed, rc = %d (%s)\n", rc, getDuckErrorString(rc));
    return;
  }
  setupOK = true;
  Serial.println("[LINK] Setup OK");
}

void loop() {
  if (!setupOK) {
    return;
  }
  duck.run();
}
