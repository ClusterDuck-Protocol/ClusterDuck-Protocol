/**
 * @file MamaDuck.ino
 * @brief Uses the built in Mama Duck.
 */

#include <string>
#include <vector>
#include <arduino-timer.h>
#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

bool sendData(std::vector<byte> message);
bool runSensor(void *);

// create a built-in mama duck
MamaDuck duck;

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 60000;
int counter = 1;
bool setupOK = false;

void setup() {
  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("MAMA0001");
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[MAMA] Failed to setup MamaDuck");
    return;
  }

  setupOK = true;

  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);
  Serial.println("[MAMA] Setup OK!");

}

std::vector<byte> stringToByteVector(const std::string& str) {
    std::vector<byte> byteVec;
    byteVec.reserve(str.length());

    for (unsigned int i = 0; i < str.length(); ++i) {
        byteVec.push_back(static_cast<byte>(str[i]));
    }

    return byteVec;
}

void loop() {
  if (!setupOK) {
    return; 
  }
  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();
}

bool runSensor(void *) {
  bool result;
  
  std::string message = std::string("Counter:") + std::to_string(counter) + " " + std::string("Free Memory:") + std::to_string(freeMemory());
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message.c_str());

  result = sendData(stringToByteVector(message));
  if (result) {
     Serial.println("[MAMA] runSensor ok.");
  } else {
     Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

bool sendData(std::vector<byte> message) {
  bool sentOk = false;
  
  int err = duck.sendData(topics::status, message);
  if (err == DUCK_ERR_NONE) {
     counter++;
     sentOk = true;
  }
  if (!sentOk) {
    std::string errMessage = "[MAMA] Failed to send data. error = " + std::to_string(err);
    Serial.println(errMessage.c_str());
  }
  return sentOk;
}
