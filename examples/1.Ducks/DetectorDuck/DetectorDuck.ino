/**
   @file DuckDetector.ino
   @author
   @brief Builds a Duck to get RSSI signal strength value.

   This example builds a duck using the preset DuckDetect to periodically send a ping message
   then provide the RSSI value of the response.

   @version
   @date 2020-09-21

   @copyright
*/

#include <arduino-timer.h>
#include <string>
#include <DuckDetect.h>

// Needed if using a board with built-in USB, such as Arduiono Zero
#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create an instance of a built-in Duck Detector
DuckDetect duck = DuckDetect();

// Create a timer with default settings
auto timer = timer_create_default();

const int INTERVAL_MS = 20000;

void setup() {
  // The Device ID must be unique and 8 bytes long. Typically the ID is stored
  // in a secure nvram, or provided to the duck during provisioning/registration
  std::vector<byte> devId = {'D', 'E', 'T', 'E', 'C', 'T', 'O', 'R'};

  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId);

  // Register  a callback that provides RSSI value
  duck.onReceiveRssi(handleReceiveRssi);

  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, pingHandler);
  Serial.println("[DETECTOR] Setup OK!");
}

void handleReceiveRssi(const int rssi) {
  showSignalQuality(rssi);
}

void loop() {
  timer.tick();
  duck.run(); // use internal duck detect behavior
}

// Periodically sends a ping message
bool pingHandler(void *) {
  Serial.println("[DETECTOR] Says ping!");
  duck.sendPing(true);

  return true;
}

// This uses the serial console to output the RSSI quality
// But you can use a display, sound or LEDs
void showSignalQuality(int incoming) {
  int rssi = incoming;
  Serial.print("[DETECTOR] Rssi value: ");
  Serial.print(rssi);

  if (rssi > -95) {
    Serial.println(" - GOOD");
  }
  else if (rssi <= -95 && rssi > -108) {
    Serial.println(" - OKAY");
  }
  else if (rssi <= -108) {
    Serial.println(" - BAD");
  }
}
