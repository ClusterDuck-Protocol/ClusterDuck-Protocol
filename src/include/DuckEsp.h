#ifndef DUCKESP_H_
#define DUCKESP_H_

#include "cdpcfg.h"
#include <Arduino.h>
#include <WString.h>

namespace duckesp {

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
String getDuckMacAddress(boolean format);
} // namespace duckesp
#endif