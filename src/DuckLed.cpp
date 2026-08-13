#include "DuckLed.h"

DuckLed* DuckLed::instance = NULL;

DuckLed::DuckLed() {}

DuckLed* DuckLed::getInstance() {
  return (instance == NULL) ? new DuckLed : instance;
}

void DuckLed::setupLED(int redPin, int greenPin, int bluePin) {
#ifdef ESP32
  this->redPin = redPin;
  this->greenPin = greenPin;
  this->bluePin = bluePin;

  // 12 kHz PWM, 8-bit resolution
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // arduino-esp32 >= 3.0: pin-based LEDC API, no explicit channel.
  ledcAttach(redPin, 12000, 8);
  ledcAttach(greenPin, 12000, 8);
  ledcAttach(bluePin, 12000, 8);
#else
  // arduino-esp32 < 3.0: channel-based LEDC API.
  ledcSetup(kRedChannel, 12000, 8);
  ledcSetup(kGreenChannel, 12000, 8);
  ledcSetup(kBlueChannel, 12000, 8);
  ledcAttachPin(redPin, kRedChannel);
  ledcAttachPin(greenPin, kGreenChannel);
  ledcAttachPin(bluePin, kBlueChannel);
#endif
#endif
}

void DuckLed::setColor(int ledR, int ledG, int ledB) {
#ifdef ESP32
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(redPin, ledR);
  ledcWrite(greenPin, ledG);
  ledcWrite(bluePin, ledB);
#else
  ledcWrite(kRedChannel, ledR);
  ledcWrite(kGreenChannel, ledG);
  ledcWrite(kBlueChannel, ledB);
#endif
#endif
}
