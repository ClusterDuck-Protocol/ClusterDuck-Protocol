#include "include/DuckUtils.h"

namespace duckutils {

volatile bool enableDuckInterrupt = true;
Timer<> duckTimer = timer_create_default();

volatile bool getDuckInterrupt() { return enableDuckInterrupt; }
void setDuckInterrupt(bool interrupt) { enableDuckInterrupt = interrupt; }

Timer<> getTimer() { return duckTimer; }

String createUuid(int length) {
  String msg = "";
  int i;

  for (i = 0; i < length; i++) {
    byte randomValue = random(0, 36);
    if (randomValue < 26) {
      msg = msg + char(randomValue + 'a');
    } else {
      msg = msg + char((randomValue - 26) + '0');
    }
  }
  return msg;
}

String convertToHex(byte* data, int size) {
  String buf = "";
  buf.reserve(size * 2);
  const char* cs = "0123456789abcdef";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0f];
    buf += cs[val & 0x0f];
  }
  return buf;
}
} // namespace duckutils