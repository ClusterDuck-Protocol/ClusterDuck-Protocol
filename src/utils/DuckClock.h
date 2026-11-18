/**
 * @file DuckClock.h
 * @brief This file is internal to CDP and provides access to the system clock.
 *
 * CDP reads and sets the wall clock only through these functions, so the host
 * tests can replace them with a fake clock. On the ESP32 the system clock keeps
 * running through deep sleep and software resets, and starts again at 1970
 * after a power loss.
 */

#ifndef DUCKCLOCK_H_
#define DUCKCLOCK_H_

#include <stdint.h>
#include <time.h>

namespace duckclock {

/**
 * @brief Get the current time from the system clock.
 *
 * @returns UTC seconds since 1970.
 */
time_t now();

/**
 * @brief Set the system clock.
 *
 * @param epoch UTC seconds since 1970
 * @returns 0 on success, -1 on failure.
 */
int set(uint32_t epoch);

} // namespace duckclock

#endif
