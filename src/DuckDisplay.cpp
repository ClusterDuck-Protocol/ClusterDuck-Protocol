#include "DuckDisplay.h"

#ifdef CDPCFG_OLED_CLASS

// #define CDPCFG_PIN_OLED_ROTATION U8G2_R0

CDPCFG_OLED_CLASS u8g2(            CDPCFG_PIN_OLED_ROTATION,
                        /* clock=*/CDPCFG_PIN_OLED_CLOCK,
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
  u8g2.begin();
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.drawStr(0,10,"Hello World!");  // write something to the internal memory
  u8g2.sendBuffer();  
  if (duckType >= DuckType::MAX_TYPE) {
    this->duckType = DuckType::UNKNOWN;
  } else {
    this->duckType = duckType;
  }
  this->duid = duid;
}

void DuckDisplay::powerSave(bool save) {
  if (save) {
    u8g2.clear();
  } else {
    u8g2.initDisplay();
  }
}

void DuckDisplay::drawString(u8g2_uint_t x, u8g2_uint_t y, const char *s) {
  u8g2.drawStr(x, y, s);
}

void DuckDisplay::drawString(bool cls, u8g2_uint_t x, u8g2_uint_t y, const char *s) {
  if (cls) {
    clear();
  }
  drawString(x, y, s);
}

void DuckDisplay::setCursor(u8g2_uint_t x, u8g2_uint_t y) { u8g2.setCursor(x, y); }

void DuckDisplay::print(String s) { u8g2.print(s); }

void DuckDisplay::clear(void) { u8g2.clear(); }

String DuckDisplay::duckTypeToString(int duckType) {
  String duckTypeStr = "";
  switch (duckType) {
    case DuckType::PAPA:
      duckTypeStr = "Papa";
      break;
    case DuckType::LINK:
      duckTypeStr = "Link";
      break;
    case DuckType::DETECTOR:
      duckTypeStr = "Detector";
      break;
    case DuckType::MAMA:
      duckTypeStr = "Mama"; 
      break; 
    default:
      duckTypeStr = "Duck";
  }
  return duckTypeStr;
}

void DuckDisplay::showDefaultScreen() {

#ifdef CDPCFG_OLED_64x32
  // small display 64x32
  setCursor(0, 2);
  print("CDP");

  setCursor(0, 4);
  print("DT: " + duckTypeToString(duckType));

  setCursor(0, 5);
  print("ID: " + duid);
#else
  // default display size 128x64
  setCursor(0, 1);
  print("Clusterduck");

  setCursor(0, 2);
  print("Protocol");

  setCursor(0, 4);
  print("DT: " + duckTypeToString(duckType));

  setCursor(0, 5);
  print("ST: Online");

  setCursor(0, 6);
  print("ID: " + duid);

  setCursor(0, 7);
  print("MC: " + duckesp::getDuckMacAddress(false));
#endif
}

#endif // CDPCFG_OLED_NONE
