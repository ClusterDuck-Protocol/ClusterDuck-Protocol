/**
 * @file CdkFrame.h
 * @brief Shared CDK: text-protocol parsing helper for MamaDuck-based example
 * sketches (Heltec, Seeed Wio Tracker L1 Pro, and any future board).
 *
 * See examples/Basic-Ducks/Seeed/CDK_PROTOCOL.md for the frame format this
 * parses fields out of (comma-separated "KEY:VALUE" pairs).
 *
 * Lives under examples/Basic-Ducks/common/ (not src/) because it is
 * example-sketch glue code, not part of the CDP library itself.
 */

#ifndef CDKFRAME_H_
#define CDKFRAME_H_

#include <Arduino.h>

// Parses a single "KEY:VALUE" field out of a comma-separated CDK: frame body
// (e.g. "LAT:1.23,LNG:4.56" -> extractField(body, "LNG") returns "4.56").
// Returns an empty string if `key` isn't present.
inline String extractField(const String& body, const String& key) {
  String search = key + ":";
  int idx = body.indexOf(search);
  if (idx == -1) return "";
  int start = idx + search.length();
  int end = body.indexOf(',', start);
  return (end == -1) ? body.substring(start) : body.substring(start, end);
}

#endif  // CDKFRAME_H_
