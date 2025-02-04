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
#include "FastLED.h"

// Setup for W2812 (LED)
#define LED_TYPE WS2812
#define DATA_PIN 4
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
#include <pixeltypes.h>
CRGB leds[NUM_LEDS]; 

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// create a built-in mama duck
MamaDuck duck;
DuckDetect detect;

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 5000;
const unsigned long SIGNAL_TIMEOUT_MS = INTERVAL_MS + 1000;
unsigned long lastSignalTime = 0;
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
  timer.every(INTERVAL_MS, pingHandler);

   // Setup the LED
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(  BRIGHTNESS );
  leds[0] = CRGB::Gold;
  FastLED.show();

  Serial.println("[MAMA] Setup OK!");

}

//If in DetectorDuck mode show RSSI
void handleReceiveRssi(const int rssi) {
  if(duck.getDetectState()) { showSignalQuality(rssi); }
  lastSignalTime = millis();
}

void loop() {
  timer.tick();
  duck.run();

  // Here we check the state of our Detect flag that is adjustable using the /controlpanel
  // Changing the state will determine the Duck's behavior
  if ((millis() - lastSignalTime > SIGNAL_TIMEOUT_MS) && duck.getDetectState()) {
    Serial.println("[DETECTOR] No signal");
    leds[0] = CRGB::Red;
    FastLED.show();
    lastSignalTime = millis(); // Reset to prevent continuous messages
    detect.run();
  }
  if(!duck.getDetectState()){
    leds[0] = CRGB::Gold;
    FastLED.show();
  }
  
}

bool pingHandler(void *) {
  detect.sendPing();

  return true;
}

void showSignalQuality(int incoming) {
  int rssi = incoming;
  Serial.print("[DETECTOR] RSSI value: ");
  Serial.print(rssi);

  if (rssi > -95) {
    Serial.println(" - GOOD SIGNAL");
    leds[0] = CRGB::Green;
    FastLED.show();
  }
  else if (rssi <= -95 && rssi > -108) {
    Serial.println(" - OKAY SIGNAL");
    leds[0] = CRGB::Blue;
    FastLED.show();
  }
  else if (rssi <= -108 <= -118) {
    Serial.println(" - LOW SIGNAL");
    leds[0] = CRGB::Purple;
    FastLED.show();
  }
  else {
    Serial.println(" - NO SIGNAL");
    leds[0] = CRGB::Red;
    FastLED.show();
  }
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