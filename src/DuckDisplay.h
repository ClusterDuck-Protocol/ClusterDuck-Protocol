/**
 * @file DuckDisplay.h
 * @brief This file is internal to CDP and provides the library access to
 * display device functions. 
 * 
 * The implementation is conditioned by the `CDPCFG_OLED_NONE` flag
 * which may be defined in `cdpcfh.h` if the device display is disabled.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */
#ifndef DUCKDISPLAY_H_
#define DUCKDISPLAY_H_

#include "include/cdpcfg.h"
#include <Arduino.h>
#include <vector>
#include <string>

#ifndef CDPCFG_OLED_NONE
#include <U8g2lib.h>

#endif // CDPCFG_OLED_NONE
/**
 * @brief Internal OLED Display abstraction.
 *
 * Provides internal access to the OLED Display, so CDP can show useful
 * status information about the network and the device.
 * 
 */
class DuckDisplay {
public:
  /**
   * @brief Get the Singletom instance of the DuckDisplay class.
   * 
   * @returns A pointer to a DuckDisplay object.
   */
  static DuckDisplay* getInstance();
#ifdef CDPCFG_OLED_NONE
  void setupDisplay(int duckType, std::vector<byte> name) {}
  void powerSave(bool save) {}
  void drawString(uint8_t x, uint8_t y, const char* text) {}
  void drawString(bool cls, uint8_t x, uint8_t y, const char* text) {}
  void setCursor(uint8_t x, uint8_t y) {}
  void print(std::string text) {}
  void clear(void) {}
  void sendBuffer(void){}
  void log(std::string text) {}
  uint8_t getWidth() { return 0; }
  uint8_t getHeight() { return 0; }
  void showDefaultScreen(){};
#else
  /**
   * @brief Initialize the display component.
   * 
   */
  void setupDisplay(int duckType, std::vector<byte> name);
  /**
   * @brief Toggle the display in or out of power saving mode.
   * 
   * @param save Set to true to enable power saving, false to disable
   */
  void powerSave(bool save);
  /**
   * @brief Draw a string at the given coordinates.
   *
   * @param x     value of X coordinate
   * @param y     value of Y coordinate
   * @param text  string to draw
   */
  void drawString(u8g2_uint_t x, u8g2_uint_t y, const char *s);
  /**
   * @brief Draw a string at the given coordinates.
   *
   * @param cls   when set to true, will clear the screen, before drawing
   * @param x     value of X coordinate
   * @param y     value of Y coordinate
   * @param text  string to draw
   */
  void drawString(bool cls, u8g2_uint_t x, u8g2_uint_t y, const char *s);
  /**
   * @brief Set the cursor to the given position on the screen.
   *
   * @param x X coordinate value
   * @param y Y coordinate value
   */
  void setCursor(u8g2_uint_t x, u8g2_uint_t y);
  /**
   * @brief Print a string at the current cursor position.
   *
   * @param text string to draw
   */
  void print(std::string s);
  /**
   * @brief Clear the screen.
   * 
   */
  void clear(void);

  void clearLine(u8g2_uint_t x, u8g2_uint_t y);

  void sendBuffer(void);
  void showDefaultScreen();
  uint8_t getWidth() {return width;}
  uint8_t getHeight() {return height;}
  
  //TODO implement this for the U8g2 library
  void log(std::string text) {}
#endif // CDPCFG_OLED_NONE
private:
  DuckDisplay();
  DuckDisplay(DuckDisplay const&) = delete;
  DuckDisplay& operator=(DuckDisplay const&) = delete;
  static DuckDisplay* instance;

#ifndef CDPCFG_OLED_NONE
  int duckType;
  std::string duckName;
  std::string duckTypeToString(int duckType);
  uint8_t width;
  uint8_t height;
#endif // CDPCFG_OLED_NONE
};

#endif /* DUCKDISPLAY_H_ */
