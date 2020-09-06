#ifndef DUCKUTILS_H_
#define DUCKUTILS_H_

#include "cdpcfg.h"
#include <Arduino.h>
#include <WString.h>

namespace duckutils {

String createUuid();
String convertToHex(byte* data, int size);


} // namespace duckutils
#endif