#ifndef DUCKDISPLAY_H_
#define DUCKDISPLAY_H_

#include "cdpcfg.h"

#include <Arduino.h>
#include <WString.h>
#include <U8x8lib.h>

class DuckDisplay {
public:
	DuckDisplay();


	static void setupDisplay();
    static void powerSave(bool save);
	static void drawString(uint8_t x, uint8_t y, const char* text);
	static void drawString(bool cls, uint8_t x, uint8_t y, const char* text);
	static void setCursor(uint8_t x, uint8_t y);
	static void print(String text);
	static void clear(void);
};

#endif /* DUCKDISPLAY_H_ */
