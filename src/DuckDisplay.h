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
#ifndef CDPCFG_OLED_NONE
#include <U8x8lib.h>
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
  void drawString(uint8_t x, uint8_t y, const char* text) {}
  void drawString(bool cls, uint8_t x, uint8_t y, const char* text) {}
  void setCursor(uint8_t x, uint8_t y) {}
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

  void displayExample(){
 


  drawString( 0,6, duckTypeString.c_str() );
  drawString( 0,1, "ID:" );
  drawString(5,1, duid.c_str());
  drawString(0,2, "Version: " );
  drawString(10,2, "2.1.1");
  drawString(0,3, "Mac: ");
  drawString(5,3, "12:33:44:55");

  

  };

  String duckTypeString(int duckType) {

  switch(duckType) {
    case 1: duckType = 0x01; 
      duckTypeStr = "Papa";
      break;
  case 2: duckType = 0x02; 
   duckTypeStr = "Mama";
      break;
    default:
      duckTypeStr = "Duck";
  }
  return duckTypeStr;
};

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

#endif
private:
  DuckDisplay();
  DuckDisplay(DuckDisplay const&) = delete;
  DuckDisplay& operator=(DuckDisplay const&) = delete;
  static DuckDisplay* instance;
  int duckType;
  String duid;
  String duckTypeStr;
 
};

#endif /* DUCKDISPLAY_H_ */
