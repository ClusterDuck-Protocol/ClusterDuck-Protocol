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
#include <WString.h>
namespace duckutils {

extern volatile bool busyDuck;
extern Timer<> duckTimer;

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
 * @brief Get the Duck Interrupt state
 * 
 * @returns true if interrupt is enabled, false otherwise.
 */
volatile bool isDuckBusy();

/**
 * @brief Toggle the duck Interrupt
 * 
 * @param busy true to enable interrupt, false otherwise.
 */
void setDuckBusy(bool busy);

Timer<> getTimer();

} // namespace duckutils
#endif