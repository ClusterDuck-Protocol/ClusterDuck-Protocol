#include "include/DuckUtils.h"

namespace duckutils {

volatile bool enableDuckInterrupt = true;
Timer<> duckTimer = timer_create_default();

volatile bool getDuckInterrupt() { return enableDuckInterrupt; }
void setDuckInterrupt(bool interrupt) { enableDuckInterrupt = interrupt; }

Timer<> getTimer() { return duckTimer; }

void  getRandomBytes(int length, byte* bytes) {
  const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;


  for (i = 0; i < length; i++) {
    bytes[i] = digits[random(0, 35)];
  }
}

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
  buf.reserve(size * 4);
  const char* cs = "0123456789ABCDEF";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0F];
    buf += cs[val & 0x0F];
  }
  return buf;
}

} // namespace duckutils