
/**
 * This example creates a duck link that sends a counter message periodically
 * It's using a pre-built ducklink available from the ClsuterDuck SDK 
 */

#include <arduino-timer.h>
#include <string>
#include <DuckLink.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in duck link
DuckLink duck = DuckLink();

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 30000;
int counter = 1;

void setup() {
  // The Device ID must be unique and 8 bytes long. Typically the ID is stored
  // in a secure nvram, or provided to the duck during provisioning/registration
  std::vector<byte> devId = {'D', 'U', 'C', 'K', 'L', 'I', 'N', 'K'};

  // Use the default setup provided by the SDK
  duck.setupWithDefaults(devId);

  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);
  Serial.println("[LINK] Setup OK!");

}

void loop() {
  timer.tick();
  duck.run();
}

bool runSensor(void *) {
  bool result = false;
  const byte* buffer;
  
  String message = String("link samd:") + String(counter);
  Serial.print("[LINK] sensor data: ");
  Serial.println(message);
  
  // There different way of sending data. Here we simply pass in the String object
  int err = duck.sendData(topics::status, message);
  if (err == DUCK_ERR_NONE) {
     result = true;
     counter++;
  } else {
    Serial.println("[LINK] Failed to send data. error = " + String(err));
  }
  return result;
}
