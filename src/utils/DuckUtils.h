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
#include <Arduino.h>
#include <EEPROM.h>
#include <string>
#include <vector>
#include "DuckError.h"
#include <functional>
#include <thread>

namespace duckutils {

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
 * @brief Convert a byte array into an ASCII string.
 * This function assumes that the byte array contains printable ASCII characters. Intended for debugging purposes.
 * @param data a byte array to convert
 * @param size the size of the array
 * @returns A std::string representing the byte array in ASCII.
 *
 */
std::string toString(uint8_t* data, int size);

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
template<typename Container>
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
template<typename T,size_t S>
std::string arrayToHexString(const std::array<T,S> arr) {
    std::string buf = ""; // static to avoid memory leak
    buf.clear();
    buf.reserve(S * 2); // 2 digit hex
    const char* cs = "0123456789ABCDEF";
    for (int i = 0; i < S; i++) {
        uint8_t val = arr[i];
        buf += cs[(val >> 4) & 0x0F];
        buf += cs[val & 0x0F];
    }
    return buf;
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

typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long long,std::milli>> time_point_t;
/**
 * @brief Create a timer instance.
 * 
 * @returns A Timer instance.
 */
    class Timer
    {
    public:
        template <class callable = nullptr_t, class... arguments>
        Timer(unsigned int in, callable&& f, arguments&&... args)
        {
            after(in, std::forward<callable>(f), std::forward<arguments>(args)...);
        }
        template <class callable = nullptr_t, class... arguments>
        Timer(time_point_t at, callable&& f, arguments&&... args){

            auto diff= std::chrono::duration_cast<std::chrono::milliseconds>(at - now()).count();

            after(diff, std::forward<callable>(f), std::forward<arguments>(args)...);
        }
        /**
         * @brief Get the current time point in milliseconds since epoch.
         *
         * @returns A time_point_t representing the current time.
         */
        static time_point_t now() {
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch());
            return time_point_t(time);
        }

    private:
        template <class callable = nullptr_t, class... arguments>
        void after(unsigned int after, callable&& f, arguments&&... args){
            std::function<std::invoke_result_t<callable, arguments...>()> task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            task();
        }

    };
bool getDetectState();
bool flipDetectState();

}
#endif
