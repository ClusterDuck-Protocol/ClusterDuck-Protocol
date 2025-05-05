/**
 * @file DuckLink.ino
 * @author
 * @brief This example creates a DuckLink that sends a health message periodically.
 * @details The health message is sent every 10 seconds (can be set by INTERVAL_MS) 
 * using the runSensor() function. The message is a string that contains the counter 
 * value ("C") and the free memory ("FM") available on the Duck. The message is sent 
 * using the sendData function.
 */

#include <arduino-timer.h>
#include <string>
#include <vector>
#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in duck link
DuckLink duck;
bool runSensor(void *);
bool sendData(std::vector<byte> message);

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 10000;
int counter = 1;

bool setupOK = false;

void setup() {

  std::string deviceId("DUCK0001"); //deviceId MUST be 8 bytes
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[LINK] Failed to setup MamaDuck");
    return;
  }

  setupOK = true;
  
  // The timer triggers runSensor every INTERVAL_MS.
  timer.every(INTERVAL_MS, runSensor);
  
  Serial.println("[LINK] Setup OK!");
}

void loop() {
  if (!setupOK) {
    return; 
  }
  
  timer.tick();
  duck.run();
}

std::vector<byte> stringToByteVector(const std::string& str) {
    std::vector<byte> byteVec;
    byteVec.reserve(str.length());

    for (unsigned int i = 0; i < str.length(); ++i) {
        byteVec.push_back(static_cast<byte>(str[i]));
    }

    return byteVec;
}

bool runSensor(void *) {
  
  bool result = false;

  std::string message = "C:" + std::to_string(counter) + "/" + "FM:" + std::to_string(freeMemory());
  Serial.print("[LINK] sensor data: ");
  Serial.println(message.c_str());
  
  result = sendData(stringToByteVector(message));
  if (result) {
     Serial.println("[LINK] runSensor ok.");
  } else {
     Serial.println("[LINK] runSensor failed.");
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
    std::string errMessage = "[LINK] Failed to send data. error = " + std::to_string(err);
    Serial.println(errMessage.c_str());
  }
  return sentOk;
}