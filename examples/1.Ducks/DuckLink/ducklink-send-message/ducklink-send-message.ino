
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
  // We are using a hardcoded device id here, but it should be retrieved or given during the device provisioning
  // then converted to a byte vector to setup the duck
  // NOTE: The Device ID must be exactly 8 bytes otherwise it will get rejected
  std::string deviceId("LINK0003");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

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
  bool result;
  const byte* buffer;
  
  String message = String("link0003:") + String(counter);
  int length = message.length();
  Serial.print("[LINK] sensor data: ");
  Serial.println(message);
  buffer = (byte*) message.c_str(); 

  result = sendData(buffer, length);
  if (result) {
     Serial.println("[LINK] runSensor ok.");
  } else {
     Serial.println("[LINK] runSensor failed.");
  }
  return result;
}

bool sendData(const byte* buffer, int length) {
  bool sentOk = false;
  
  // Send Data can either take a byte buffer (unsigned char) or a vector
  int err = duck.sendData(topics::status, buffer, length);
  if (err == DUCK_ERR_NONE) {
     counter++;
     sentOk = true;
  }
  if (!sentOk) {
    Serial.println("[LINK] Failed to send data. error = " + String(err));
  }
  return sentOk;
}
