/**
 * @file DuckIdFactory.h
 * @brief Shared factory-MAC-derived Duck ID generation for MamaDuck-based
 * example sketches (Heltec, Seeed Wio Tracker L1 Pro, and any future board).
 *
 * Lives under examples/Basic-Ducks/common/ (not src/) because it is
 * example-sketch glue code, not part of the CDP library itself.
 */

#ifndef DUCKIDFACTORY_H_
#define DUCKIDFACTORY_H_

#include <cstring>
#include <string>
#include "DuckEsp.h"

namespace duckidfactory {

// Derives an 8-character Duck ID from this board's factory-unique MAC/efuse
// address (see duckesp::getDuckMacAddress()) and writes it, with a null
// terminator, into buf9 (which must be at least 9 bytes). Used by every
// MamaDuck-based example so each device gets a distinct, reboot-stable ID
// with no manual configuration required, unless the sketch defines a fixed
// DUCK_ID instead.
inline void deriveFromMac(char* buf9) {
  std::string mac = duckesp::getDuckMacAddress(false);  // unformatted hex, e.g. "E4B4C2A1B2C3"
  std::string id  = (mac.length() >= 8) ? mac.substr(mac.length() - 8) : std::string("DUCK0000");
  memcpy(buf9, id.c_str(), 8);
  buf9[8] = '\0';
}

}  // namespace duckidfactory

#endif  // DUCKIDFACTORY_H_
