#include "DuckLed.h"

int DuckLed::ledR = CDPCFG_PIN_RGBLED_R;
int DuckLed::ledG = CDPCFG_PIN_RGBLED_G;
int DuckLed::ledB = CDPCFG_PIN_RGBLED_B;

DuckLed::DuckLed() {

}

void  DuckLed::setupLED() {
#ifdef ESP32
	ledcAttachPin(ledR, 1); // assign RGB led pins to channels
	ledcAttachPin(ledG, 2);
	ledcAttachPin(ledB, 3);

	// Initialize channels
	// channels 0-15, resolution 1-16 bits, freq limits depend on resolution
	// ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
	ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
	ledcSetup(2, 12000, 8);
	ledcSetup(3, 12000, 8);
#endif
}

void DuckLed::setColor(int ledR, int ledG, int ledB) {
#ifdef ESP32
	ledcWrite(1, ledR);
	ledcWrite(2, ledG);
	ledcWrite(3, ledB);
#endif
}

