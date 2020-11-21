#ifndef DUCKLINK_H
#define DUCKLINK_H

#include <Arduino.h>
#include <WString.h>

#include "include/Duck.h"
#include "include/cdpcfg.h"

class DuckLink : public Duck {
public:
  using Duck::Duck;
  
  ~DuckLink() {}

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
   * 
   * @returns DUCK_ERR_NONE if setup is successfull, an error code otherwise.
   */
  int setupWithDefaults(std::vector<byte> deviceId, String ssid="", String password="");

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  int getType() { return DuckType::LINK; }
};

#endif
