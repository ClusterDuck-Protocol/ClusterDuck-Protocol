# 1 "/tmp/tmph9a4qrcv"
#include <Arduino.h>
# 1 "/home/faradaym/Arduino/libraries/ClusterDuck-Protocol/examples/Basic-Ducks/MamaDuck/MamaDuck.ino"
# 12 "/home/faradaym/Arduino/libraries/ClusterDuck-Protocol/examples/Basic-Ducks/MamaDuck/MamaDuck.ino"
#include <string>
#include <arduino-timer.h>
#include <CDP.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif


bool sendData(std::string message, byte topic=topics::status);
bool runSensor(void *);


MamaDuck duck;
auto timer = timer_create_default();
const int INTERVAL_MS = 10000;
int counter = 1;
bool setupOK = false;
# 38 "/home/faradaym/Arduino/libraries/ClusterDuck-Protocol/examples/Basic-Ducks/MamaDuck/MamaDuck.ino"
void setup();
void loop();
#line 38 "/home/faradaym/Arduino/libraries/ClusterDuck-Protocol/examples/Basic-Ducks/MamaDuck/MamaDuck.ino"
void setup() {

  std::string deviceId("MAMA0001");
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[MAMA] Failed to setup MamaDuck");
    return;
  }

  timer.every(INTERVAL_MS, runSensor);

  setupOK = true;
  Serial.println("[MAMA] Setup OK!");
}






void loop() {
  if (!setupOK) {
    return;
  }
  timer.tick();

  duck.run();
}
# 77 "/home/faradaym/Arduino/libraries/ClusterDuck-Protocol/examples/Basic-Ducks/MamaDuck/MamaDuck.ino"
bool runSensor(void *) {
  bool result;

  std::string message = "C:" + std::to_string(counter) + "|" + "FM:" + std::to_string(freeMemory());
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message.c_str());

  result = sendData(message, topics::health);
  if (result) {
    counter++;
    Serial.println("[MAMA] runSensor ok.");
  } else {
    Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}
# 103 "/home/faradaym/Arduino/libraries/ClusterDuck-Protocol/examples/Basic-Ducks/MamaDuck/MamaDuck.ino"
bool sendData(std::string message, byte topic) {
  bool sentOk = false;

  int err = duck.sendData(topic, message);
  if (err == DUCK_ERR_NONE) {
    sentOk = true;
  }
  if (!sentOk) {
    Serial.println(("[Link] Failed to send data. error = " + std::to_string(err)).c_str());
  }
  return sentOk;
}