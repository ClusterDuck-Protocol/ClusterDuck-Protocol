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
#endif
private:
  DuckDisplay();
  DuckDisplay(DuckDisplay const&) = delete;
  DuckDisplay& operator=(DuckDisplay const&) = delete;
  static DuckDisplay* instance;
  int duckType;
  String duid;
  String duckTypeToString(int duckType);

};
#endif /* DUCKDISPLAY_H_ */