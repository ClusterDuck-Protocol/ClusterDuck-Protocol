#include "DuckClock.h"
#include <sys/time.h>

namespace duckclock {

time_t now() {
  return time(nullptr);
}

int set(uint32_t epoch) {
  timeval tv{};
  tv.tv_sec = static_cast<time_t>(epoch);
  return settimeofday(&tv, nullptr);
}

} // namespace duckclock
