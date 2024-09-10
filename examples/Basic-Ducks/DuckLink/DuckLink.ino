
/**
 * @file DuckLink.ino
 * @author
 * @brief This example creates a duck link that sends a counter message periodically.
 *
 * It's using a pre-built DuckLink available from the ClusterDuck SDK
 */

#include <arduino-timer.h>
#include <string>
#include <CDP.h>


#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in duck link
DuckLink duck;

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 30000;
int counter = 1;

bool setupOK = false;

void setup() {
  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("DUCK0001");
  std::vector<byte> devId;
  int rc;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  rc = duck.setupWithDefaults(devId);
  if (rc != DUCK_ERR_NONE) {
    Serial.print("[LINK] Failed to setup ducklink: rc = ");Serial .println(rc);
    return;
  }

  setupOK = true;
  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  Serial.println("[LINK] Setup OK!");

  if (sendSensorData()) {
    timer.every(INTERVAL_MS, runSensor);
  } else {
    Serial.println("[LINK] ERROR - Failed to send sensor data");
  }
}

void loop() {
  if (!setupOK) {
    return; 
  }
  
  timer.tick();
  duck.run();
}

std::vector<byte> stringToByteVector(const String& str) {
    std::vector<byte> byteVec;
    byteVec.reserve(str.length());

    for (unsigned int i = 0; i < str.length(); ++i) {
        byteVec.push_back(static_cast<byte>(str[i]));
    }

    return byteVec;
}

bool sendSensorData() {
  bool result = false;
  const byte* buffer;
  
  String message = String("Counter:") + String(counter);
  Serial.print("[LINK] sensor data: ");
  Serial.println(message);
  
  int err = duck.sendData(topics::status, stringToByteVector(message));
  if (err == DUCK_ERR_NONE) {
     result = true;
     counter++;
  } else {
    Serial.println("[LINK] Failed to send data. error = " + String(err));
    return false;
  }
  return result;
}

bool runSensor(void *) {
  return sendSensorData();
}