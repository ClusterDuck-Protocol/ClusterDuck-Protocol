#ifndef DUCKUTILS_H_
#define DUCKUTILS_H_

#include "cdpcfg.h"
#include <Arduino.h>
#include <WString.h>
#include "timer.h"
namespace duckutils {

extern volatile bool enableDuckInterrupt;
extern Timer<> duckTimer;

String createUuid();
String convertToHex(byte* data, int size);


volatile bool getDuckInterrupt();
void setDuckInterrupt(bool interrupt);

Timer<> getTimer();

} // namespace duckutils
#endif