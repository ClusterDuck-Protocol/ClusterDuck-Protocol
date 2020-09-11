#ifndef DUCKESP_H_
#define DUCKESP_H_

#include "cdpcfg.h"
#include <Arduino.h>
#include <WString.h>

namespace duckesp {

void restartDuck();
String getDuckMacAddress(boolean format);
} // namespace duckesp
#endif