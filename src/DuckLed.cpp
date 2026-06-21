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
  ledcAttach(redPin, 12000, 8);
  ledcAttach(greenPin, 12000, 8);
  ledcAttach(bluePin, 12000, 8);
#endif
}

void DuckLed::setColor(int ledR, int ledG, int ledB) {
#ifdef ESP32
  ledcWrite(redPin, ledR);
  ledcWrite(greenPin, ledG);
  ledcWrite(bluePin, ledB);
#endif
}
