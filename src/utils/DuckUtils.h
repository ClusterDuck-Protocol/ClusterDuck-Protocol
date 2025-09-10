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

#include "../include/cdpcfg.h"
#include "arduino-timer.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string>
#include <vector>
#include "DuckError.h"

namespace duckutils {

extern Timer<> duckTimer;
extern bool detectState;

std::string getCDPVersion();

/**
 * @brief Convert a string to upper case.
 *
 * @param str the string to convert
 * @returns A string in upper case.
 */
std::string toUpperCase(std::string str);

/**
 * @brief Convert a string to a byte array.
 *
 * @param str the string to convert
 * @returns A vector of bytes.
 */
std::vector<uint8_t> stringToByteVector(const std::string& str);

/**
 * @brief Creates a byte array with random alpha numerical values.
 *
 * @param length the array length
 * @param bytes array of bytes of the specified length
 */ 
void getRandomBytes(int length, uint8_t* bytes);

/**
 * @brief Create a uuid string.
 *
 * @param length the length of the UUID (defaults to CDPCFG_UUID_LEN)
 * @returns A string representing a unique id.
 */
std::string createUuid(int length = CDPCFG_UUID_LEN);

/**
 * @brief Convert a byte array into a hex string.
 * 
 * @param data a byte array to convert
 * @param size the size of the array
 * @returns A string representating the byte array in hexadecimal.
 */
std::string convertToHex(uint8_t* data, int size);

/**
 * @brief Convert a vector into an ASCII string.
 * 
 * @param vec A vector to convert
 * @returns A std::string representing the byte array in ASCII.
 * 
 */
template<typename T>
std::string toString(const std::vector<T>& vec) {
    std::string result;
    for (const auto& element : vec) {
        if (!std::isprint(element)) {
            return "ERROR: Non-printable character";
        }
        result += static_cast<char>(element);
    }
    return result;
}

/**
 * @brief Convert an array into an ASCII string.
 *
 * @param arr An array to convert
 * @returns A std::string representing the byte array in ASCII.
 *
 */
template<typename Container,size_t S>
std::string toString(const Container& arr) {
    std::string result;
    for (const auto& element : arr) {
        if (!std::isprint(element)) {
            return "ERROR: Non-printable character";
        }
        result += static_cast<char>(element);
    }
    return result;
}
/**
 * @brief Convert a string into an array of bytes.
 *
 * @param str the string to convert
 * @returns A std::array of bytes representing the string.
 * @throws std::out_of_range if the string size exceeds the array size.
 */
template<typename T,size_t S>
std::array<T,S> stringToArray(const std::string& str) {
    std::array<T,S> arr;
    if (str.size() > S) {
        throw std::out_of_range("String size exceeds array size");
    }
    for (size_t i = 0; i < str.size(); ++i) {
        arr[i] = static_cast<T>(str[i]);
    }
    return arr;
}

/**
 * @brief Convert an container of contiguous data into hex for sending over http or display.
 * May be more ergonomic than the convertToHex function.
 * @param con a container to convert
 * @returns A std::string representing the byte array in ASCII.
 *
 */
template<typename Container>
std::string containerToHexString(Container& con) {
        convertToHex(con.data(), con.size());
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

    template<typename T,size_t S>
    bool isEqual(const std::array<T,S> & a, const std::array<T,S> & b) {
        if (a.size() != b.size()) {
            return false;
        }
        return std::equal(a.begin(), a.end(), b.begin());
    }

/**
 * @brief Convert a byte array to unsigned 32 bit integer.
 * 
 * @param data byte array to convert
 * @returns a 32 bit unsigned integer.
 */
uint32_t toUint32(const uint8_t* data);

/**
 * @brief Get a timer instance.
 * 
 * @returns A Timer instance.
 */
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
int saveWifiCredentials(std::string ssid, std::string password);

/**
 * @brief Load WiFi SSID from EEPROM
 *
 * @returns A string representing the WiFi SSID
 */
std::string loadWifiSsid();

/**
 * @brief Load WiFi password from EEPROM
 *
 * @returns A string representing the WiFi password
 */
std::string loadWifiPassword();

} // namespace duckutils
#endif
