/**
 * @file DuckUtils.h
 * @brief This file is internal to CDP and provides some common utility methods.
 * @version
 * @date 2020-09-16
 *
 * @copyright
 */

#ifndef DUCKUTILS_H_
#define DUCKUTILS_H_

#include "cdpcfg.h"
#include "arduino-timer.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string>
#include <WString.h>
#include <vector>

#include "../DuckError.h"

namespace duckutils {

extern Timer<> duckTimer;
extern bool detectState;

std::string getCDPVersion();

/**
 * @brief Creates a byte array with random alpha numerical values.
 *
 * @param length the array length
 * @param bytes array of bytes of the specified length
 */ 
void getRandomBytes(int length, byte* bytes);

/**
 * @brief Create a uuid string.
 *
 * @param length the length of the UUID (defaults to CDPCFG_UUID_LEN)
 * @returns A string representing a unique id.
 */
String createUuid(int length = CDPCFG_UUID_LEN);

/**
 * @brief Convert a byte array into a hex string.
 * 
 * @param data a byte array to convert
 * @param size the size of the array
 * @returns A string representating the by array in hexadecimal.
 */
String convertToHex(byte* data, int size);

/**
 * @brief Convert a vector into an ASCII string.
 * 
 * @param vec A vector to convert
 * @returns A String representing the byte array in ASCII.
 * 
 */
template<typename T>
String toString(const std::vector<T> & vec) {
  return std::string(vec.begin(), vec.end()).c_str();
}

/**
 * @brief Compare two vectors with regard to both size and contents.
 * 
 * @param a The first vector
 * @param b The second vector
 * @returns True if a and b have the same size and contents, false if not.
 */
template<typename T>
bool isEqual(const std::vector<T> & a, const std::vector<T> & b) {
  if (a.size() != b.size()) {
    return false;
  }
  return std::equal(a.begin(), a.end(), b.begin());
}

/**
 * @brief Toggle the duck Interrupt
 *
 * @param enable true to enable interrupt, false otherwise.
 */
void setInterrupt(bool enable);

/**
 * @brief Convert a byte array to unsigned 32 bit integer.
 * 
 * @param data byte array to convert
 * @returns a 32 bit unsigned integer.
 */
uint32_t toUnit32(const byte* data);

Timer<> getTimer();

bool getDetectState();
bool flipDetectState();

/**
 * @brief Save / Write Wifi credentials to EEPROM
 *
 * @param ssid        the ssid of the WiFi network
 * @param password    password to join the network
 * @return DUCK_ERR_NONE if successful, an error code otherwise.
 */
int saveWifiCredentials(String ssid, String password);

/**
 * @brief Load WiFi SSID from EEPROM
 *
 * @returns A string representing the WiFi SSID
 */
String loadWifiSsid();

/**
 * @brief Load WiFi password from EEPROM
 *
 * @returns A string representing the WiFi password
 */
String loadWifiPassword();

} // namespace duckutils
#endif
