#include "include/DuckDisplay.h"

#ifdef CDPCFG_OLED_CLASS
CDPCFG_OLED_CLASS u8x8(/* clock=*/CDPCFG_PIN_OLED_CLOCK,
                       /* data=*/CDPCFG_PIN_OLED_DATA,
                       /* reset=*/CDPCFG_PIN_OLED_RESET);
#endif

DuckDisplay* DuckDisplay::instance = NULL;

DuckDisplay::DuckDisplay() {}

DuckDisplay* DuckDisplay::getInstance() {
  if (!instance)
    instance = new DuckDisplay;
  return instance;
}

void DuckDisplay::setupDisplay() {
#ifndef CDPCFG_OLED_NONE
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
#endif
}

void DuckDisplay::powerSave(bool save) {
#ifndef CDPCFG_OLED_NONE
  if (save) {
    u8x8.noDisplay();
  } else {
    u8x8.display();
  }
#endif
}

void DuckDisplay::drawString(uint8_t x, uint8_t y, const char* text) {
#ifndef CDPCFG_OLED_NONE
  u8x8.drawString(x, y, text);
#endif
}

void DuckDisplay::drawString(bool cls, uint8_t x, uint8_t y, const char* text) {
  if (cls) {
    clear();
  }
  drawString(x, y, text);
}

void DuckDisplay::setCursor(uint8_t x, uint8_t y) {
#ifndef CDPCFG_OLED_NONE
  u8x8.setCursor(x, y);
#endif
}

void DuckDisplay::print(String text) {
#ifndef CDPCFG_OLED_NONE
  u8x8.print(text);
#endif
}

void DuckDisplay::clear(void) {
#ifndef CDPCFG_OLED_NONE
  u8x8.clear();
#endif
}
