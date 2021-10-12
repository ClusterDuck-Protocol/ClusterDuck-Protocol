#include "include/DuckUtils.h"
#include <iomanip>
#include <sstream>
#include <string>

namespace duckutils {

  namespace {
    std::string cdpVersion = "2.10.2";
  }

Timer<> duckTimer = timer_create_default();
bool detectState = false;

std::string getCDPVersion() {
  return cdpVersion;
}

Timer<> getTimer() { return duckTimer; }

bool getDetectState() { return detectState; }
bool flipDetectState() {
  detectState = !detectState;
  return detectState;

}

void  getRandomBytes(int length, byte* bytes) {
  const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;
  for (i = 0; i < length; i++) {
    //TODO: Random generator here isn't seeded properly
    //We can use RSSI value to seed it or use a frame counter if available
    bytes[i] = digits[random(36)];    
  }
}

String createUuid(int length) {
  String msg = "";
  int i;

  for (i = 0; i < length; i++) {
    byte randomValue = random(36);
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
  buf.reserve(size * 2); // 2 digit hex
  const char* cs = "0123456789ABCDEF";
  for (int i = 0; i < size; i++) {
    byte val = data[i];
    buf += cs[(val >> 4) & 0x0F];
    buf += cs[val & 0x0F];
  }
  return buf;
}

uint32_t toUnit32(const byte* data) {
    uint32_t value = 0;

    value |= data[0] << 24;
    value |= data[1] << 16;
    value |= data[2] << 8;
    value |= data[3];
    return value;
}

} // namespace duckutils
