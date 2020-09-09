#ifndef DUCKDISPLAY_H_
#define DUCKDISPLAY_H_

#include <Arduino.h>
#include <U8x8lib.h>
#include <WString.h>

#include "cdpcfg.h"

class DuckDisplay {
public:
  static DuckDisplay* getInstance();
  

  void setupDisplay();
  void powerSave(bool save);
  void drawString(uint8_t x, uint8_t y, const char* text);
  void drawString(bool cls, uint8_t x, uint8_t y, const char* text);
  void setCursor(uint8_t x, uint8_t y);
  void print(String text);
  void clear(void);

private:
  DuckDisplay();
  DuckDisplay(DuckDisplay const&) = delete;   
  DuckDisplay&operator=(DuckDisplay const&) = delete; 
  static DuckDisplay* instance;
};

#endif /* DUCKDISPLAY_H_ */
