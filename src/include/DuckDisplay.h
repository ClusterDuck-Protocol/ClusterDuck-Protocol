/**
 * @file DuckDisplay.h
 * @author
 * @brief This file is internal to CDP and provides the library access to
 * display device functions. The implementation is conditioned by the
 * CDPCFG_OLED_NONE flag which may be defined in cdpcfh.h if the device does not
 * have any display.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */

#ifndef DUCKDISPLAY_H_
#define DUCKDISPLAY_H_

#include <Arduino.h>
#include <U8x8lib.h>
#include <WString.h>

#include "cdpcfg.h"

/**
 * OLED Display abstraction.
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
   * @return A pointer to a DuckDisplay object.
   */
  static DuckDisplay* getInstance();

  /**
   * @brief Initialize the display component.
   * 
   */
  void setupDisplay();
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
  void drawString(uint8_t x, uint8_t y, const char* text);

  /**
   * @brief Draw a string at the given coordinates.
   *
   * @param cls   when set to true, will clear the screen, before drawing
   * @param x     value of X coordinate
   * @param y     value of Y coordinate
   * @param text  string to draw
   */
  void drawString(bool cls, uint8_t x, uint8_t y, const char* text);

  /**
   * @brief Set the cursor to the given position on the screen.
   *
   * @param x X coordinate value
   * @param y Y coordinate value
   */
  void setCursor(uint8_t x, uint8_t y);

  /**
   * @brief Print a string at the current cursor position.
   *
   * @param text string to draw
   */
  void print(String text);

  /**
   * @brief Clear the screen.
   * 
   */
  void clear(void);

private:
  DuckDisplay();
  DuckDisplay(DuckDisplay const&) = delete;
  DuckDisplay& operator=(DuckDisplay const&) = delete;
  static DuckDisplay* instance;
};

#endif /* DUCKDISPLAY_H_ */
