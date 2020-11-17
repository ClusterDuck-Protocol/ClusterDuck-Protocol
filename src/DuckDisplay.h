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
#include <WString.h>
#include "include/DuckTypes.h"
#include "include/DuckEsp.h"
#ifndef CDPCFG_OLED_NONE
#include <U8g2lib.h>
#endif
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
  void setupDisplay(int duckType, String duid) {}
  void powerSave(bool save){}
  void drawString(u8g2_uint_t x, u8g2_uint_t y, const char *s) {}
  void drawString(bool cls,u8g2_uint_t x, u8g2_uint_t y, const char *s) {}
  void setCursor(u8g2_uint_t x, u8g2_uint_t y) {}
  void print(String text) {}
  void clear(void) {}
#else
  /**
   * @brief Initialize the display component.
   * 
   */
  void setupDisplay(int duckType, String duid);
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
  void print(String s);
  /**
   * @brief Clear the screen.
   * 
   */
  void clear(void);
  void showDefaultScreen();
  uint8_t getWidth() {return width;}
  uint8_t getHeight() {return height;}

#endif
private:
#ifdef CDPCFG_OLED_CLASS
  #define U8LOG_WIDTH 16
  #define U8LOG_HEIGHT 8
      CDPCFG_OLED_CLASS u8x8 = CDPCFG_OLED_CLASS(CDPCFG_PIN_OLED_CLOCK,
                                                 CDPCFG_PIN_OLED_DATA,
                                                 CDPCFG_PIN_OLED_RESET);
  uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
  U8X8LOG u8x8log;
#endif

  DuckDisplay();
  DuckDisplay(DuckDisplay const&) = delete;
  DuckDisplay& operator=(DuckDisplay const&) = delete;
  static DuckDisplay* instance;
  int duckType;
  String duckName;
  String duckTypeToString(int duckType);
  uint8_t width;
  uint8_t height;
};

#endif /* DUCKDISPLAY_H_ */
