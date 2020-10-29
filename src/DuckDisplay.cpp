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
  print("((>.<))");

  setCursor(0, 4);
  print("DT: " + duckTypeToString(duckType));

  setCursor(0, 5);
  print("ID: " + duid);
#else
  // default display size 128x64
  setCursor(0, 1);
  print("    ((>.<))    ");

  setCursor(0, 2);
  print("  Project OWL  ");

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
