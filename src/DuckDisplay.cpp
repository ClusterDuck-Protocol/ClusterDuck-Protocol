#include "DuckDisplay.h"
#ifdef CDPCFG_OLED_CLASS
CDPCFG_OLED_CLASS u8x8(/* clock=*/CDPCFG_PIN_OLED_CLOCK,
                       /* data=*/CDPCFG_PIN_OLED_DATA,
                       /* reset=*/CDPCFG_PIN_OLED_RESET);
#endif

DuckDisplay* DuckDisplay::instance = NULL;

DuckDisplay::DuckDisplay() {}

DuckDisplay* DuckDisplay::getInstance() {
  if (!instance) {
    instance = new DuckDisplay;
  }
  return instance;
}

#ifndef CDPCFG_OLED_NONE

void DuckDisplay::setupDisplay(int duckType, String duid) {
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  if (duckType >= DuckType::MAX_TYPE) {
    this->duckType = DuckType::UNKNOWN;
  } else {
    this->duckType = duckType;
  }
  this->duid = duid;
}

void DuckDisplay::powerSave(bool save) {
  if (save) {
    u8x8.noDisplay();
  } else {
    u8x8.display();
  }
}

void DuckDisplay::drawString(uint8_t x, uint8_t y, const char* text) {
  u8x8.drawString(x, y, text);
}

void DuckDisplay::drawString(bool cls, uint8_t x, uint8_t y, const char* text) {
  if (cls) {
    clear();
  }
  drawString(x, y, text);
}

void DuckDisplay::setCursor(uint8_t x, uint8_t y) { u8x8.setCursor(x, y); }

void DuckDisplay::print(String text) { u8x8.print(text); }

void DuckDisplay::clear(void) { u8x8.clear(); }

#endif // CDPCFG_OLED_NONE
