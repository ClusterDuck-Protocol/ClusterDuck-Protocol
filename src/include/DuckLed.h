/**
 * @file DuckLed.h
 * @brief This file is internal to CDP and provides the library access to onboard LED functions.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */

#ifndef DUCKLED_H_
#define DUCKLED_H_

#include "cdpcfg.h"
#include <Arduino.h>

/**
 * @brief Internal on board LED abstraction.
 *
 * Provides internal access to the onboard LED, so CDP can show useful
 * status information about the network and the device.
 *
 */
class DuckLed {
public:
  /**
   * @brief Get a singletom instance of the DuckLed class
   *
   * @returns A pointer to DuckLed object.
   */
  static DuckLed* getInstance();

  /**
   * @brief Set the Color object.
   *
   * @param ledR  value of the Red component. Defaults to CDPCFG_PIN_RGBLED_R
   * @param ledG  value of the Green component. Defaults to CDPCFG_PIN_RGBLED_G
   * @param ledB  value of the Blue component. Defaults to CDPCFG_PIN_RGBLED_B
   */
  void setColor(int ledR = CDPCFG_PIN_RGBLED_R, int ledG = CDPCFG_PIN_RGBLED_G,
                int ledB = CDPCFG_PIN_RGBLED_B);

  /**
   * @brief Initialize access to the LED.
   *
   * @param redPin    value of the Red pin. Defaults to CDPCFG_PIN_RGBLED_R
   * @param greenPin  value of the Green pin. Defaults to CDPCFG_PIN_RGBLED_G
   * @param bluePin   value of the Blue pin. Defaults to CDPCFG_PIN_RGBLED_B
   */
  void setupLED(int redPin = CDPCFG_PIN_RGBLED_R,
                int greenPin = CDPCFG_PIN_RGBLED_G,
                int bluePin = CDPCFG_PIN_RGBLED_B);

private:
  int redPin;
  int greenPin;
  int bluePin;

  DuckLed();
  DuckLed(DuckLed const&) = delete;
  DuckLed& operator=(DuckLed const&) = delete;
  static DuckLed* instance;
};

#endif /* DUCKLED_H_ */
