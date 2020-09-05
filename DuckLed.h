#ifndef DUCKLED_H_
#define DUCKLED_H_

#include "cdpcfg.h"

#include <Arduino.h>

class DuckLed {
public:
	DuckLed();
	static void setColor(int ledR = CDPCFG_PIN_RGBLED_R, int ledG = CDPCFG_PIN_RGBLED_G, int ledB = CDPCFG_PIN_RGBLED_B);
	static void setupLED();
private:
	static int ledR;
	static int ledG;
	static int ledB;
};

#endif /* DUCKLED_H_ */
