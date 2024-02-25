/**
 * @file DuckEsp.h
 * @brief This file is internal to CDP and provides ESP32 specific implementations.
 * @version 
 * @date 2020-09-16
 * 
 * @copyright 
 * 
 */

#ifndef DUCKESP_H_
#define DUCKESP_H_

#include "cdpcfg.h"
#include <Arduino.h>
#include <string>

namespace duckesp {

/**
 * @brief Get free heap memory
 * 
 * @returns free heap memory size in bytes 
 */
int freeHeapMemory();

/**
 * @brief Get lowest level of free heap since boot
 * 
 * @returns memory size in bytes
 */
int getMinFreeHeap();

/**
 * @brief Get largest block of heap that can be allocated at once
 * 
 * @returns memory size in bytes
 */
int getMaxAllocHeap();

/**
 * @brief Restart the duck device.
 * 
 */
void restartDuck();

/**
 * @brief Get the Duck Mac Address.
 *
 * @param format true if the mac address is formated as MM:MM:MM:SS:SS:SS
 * @return A string representing the mac address.   
 */
std::string getDuckMacAddress(boolean format);
} // namespace duckesp
#endif