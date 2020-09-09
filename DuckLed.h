#ifndef DUCKLED_H_
#define DUCKLED_H_

#include "cdpcfg.h"

#include <Arduino.h>

class DuckLed {
public:
  static DuckLed* getInstance();

  void setColor(int ledR = CDPCFG_PIN_RGBLED_R, int ledG = CDPCFG_PIN_RGBLED_G,
                int ledB = CDPCFG_PIN_RGBLED_B);

  void setupLED(int redPin = CDPCFG_PIN_RGBLED_R,
                int greenPin = CDPCFG_PIN_RGBLED_G,
                int bluePin = CDPCFG_PIN_RGBLED_B);

private:
  int redPin;
  int greenPin;
  int bluePin;

  DuckLed();
  DuckLed(DuckLed const&) = delete;
  DuckLed& operator=(DuckLed const&) = delete;
  static DuckLed* instance;
};

#endif /* DUCKLED_H_ */
