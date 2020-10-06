#ifndef DUCKLINK_H
#define DUCKLINK_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class DuckLink : public Duck {
public:
  using Duck::Duck;
  /**
   * @brief Provide the DuckLink specific implementation of the base `run()` method.
   * 
   */
  void run();
  /**
   * @brief Override the default setup method to match DuckLink specific
   * defaults.
   *
   * In addition to Serial component, the Radio component is also initialized.
   * When ssid and password are provided the duck will setup the wifi related
   * components.
   *
   * @param ssid wifi access point ssid (defaults to an empty string if not
   * provided)
   * @param password wifi password (defaults to an empty string if not provided)
   */
  void setupWithDefaults(String ssid="", String password="");
};

#endif