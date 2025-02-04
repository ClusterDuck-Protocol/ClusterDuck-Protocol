/**
 * @file Custom-Mama-Detect.ino
 * @brief Uses the built in Mama Duck and DuckDetector.
 * 
 * This example is a DubleDuck. It has the ability to easily change from a MamaDuck
 * to a DuckDetector. This can be controlled by going to 192.168.1.1/controlpanel
 * after connecting to the Duck's wifi AP. In this example the duck starts as a 
 * MamaDuck by default. 
 * 
 */

#include <string>
#include <arduino-timer.h>
#include <MamaDuck.h>
#include <DuckDetect.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in mama duck
MamaDuck duck;
DuckDetect detect;
bool runSensor(void *);
bool detectOn = false;

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 5000;
int counter = 1;

void setup() {
  // We are using a hardcoded device id here, but it should be retrieved or
  // given during the device provisioning then converted to a byte vector to
  // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
  // will get rejected
  std::string deviceId("MAMA0001");
  std::array<byte,8> devId;
  std::copy(deviceId.begin(), deviceId.end(), devId.begin());
  duck.setupWithDefaults(devId);
  detect.setDeviceId(deviceId);
  
  // Load DetectorDuck profile
  detect.setupRadio();
  detect.onReceiveRssi(handleReceiveRssi);

  // Initialize the timer. The timer thread runs separately from the main loop
  // and will trigger sending a counter message.
  timer.every(INTERVAL_MS, runSensor);
  duck.onReceiveDuckData(handleDuckData);
  Serial.println("[MAMA] Setup OK!");

}

void loop() {
  timer.tick();
  // Here we check the state of our Detect flag that is adjustable using the /controlpanel
  // Changing the state will determine the Duck's behavior
  // This strategy can be used with other ducks to morph a duck quickly into other behaviors
  
  if(duck.getDetectState()) {
    if(!detectOn) {
      detectOn = true;
      // Remove MamaDuck timer
      timer.cancel();
      detect.run();
      timer.every(3000, pingHandler);
    }
    detect.run();
  } else {
    if(detectOn) {
      detectOn = false;
      // Remove Detector timer
      timer.cancel();
      duck.run();
      timer.every(INTERVAL_MS, runSensor);
    }
    duck.run();
  }
  
}

void handleDuckData(std::vector<byte> packetBuffer) {

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
        Serial.printf("[MAMA] Failed to send data. error = %i\n", err);
    }
    return sentOk;
}

bool runSensor(void *) {
  bool result;
  const byte* buffer;
  
  std::string message = std::string("Counter:").append(std::to_string(counter));
  int length = message.length();
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message.c_str());
  buffer = (byte*) message.c_str(); 

  result = sendData(buffer, length);
  if (result) {
     Serial.println("[MAMA] runSensor ok.");
  } else {
     Serial.println("[MAMA] runSensor failed.");
  }
  return result;
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
//If in DetectorDuck mode show RSSI
void handleReceiveRssi(const int rssi) {
  if(detectOn) { showSignalQuality(rssi); }
}

// Periodically sends a ping message in DetectorDuck mode
bool pingHandler(void *) {
  Serial.println("[DETECTOR] Says ping!");
  detect.sendPing(true);

  return true;
}