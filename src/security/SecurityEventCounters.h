/**
 * @file SecurityEventCounters.h
 * @brief Lightweight, RAM-only counters for this Duck's fail-closed
 * authentication rejections.
 *
 * Incremented at each of this firmware's "reject once configured" gates:
 *  - encrypted_cmd decrypt failure (src/Ducks/MamaDuck.h)
 *  - Emergency Broadcast MAC verification failure (topic 24)
 *  - BEACON/BEACON_ACK group-key MAC verification failure
 *
 * These counters do NOT affect any accept/reject decision -- they exist
 * purely so an operator can tell "keys are fine, we're blocking forged
 * traffic" apart from "our own config/key is broken and legitimate
 * traffic is silently being dropped", without weakening any verification.
 * See docs/crypto-design.tex, fail-closed hardening discussion.
 *
 * Deliberately reported as a periodic AGGREGATE count (see summary()),
 * not one uplink per rejected packet -- so an attacker spamming forged
 * packets can't force this device to burn scarce LoRa airtime/battery
 * reporting each individual attempt.
 */

#ifndef SECURITYEVENTCOUNTERS_H_
#define SECURITYEVENTCOUNTERS_H_

#include <cstdio>
#include <cstdint>
#include <string>

namespace securityevents {

inline uint32_t encryptedCmdRejected = 0;
inline uint32_t broadcastRejected = 0;
inline uint32_t beaconRejected = 0;

inline void recordEncryptedCmdRejected() { encryptedCmdRejected++; }
inline void recordBroadcastRejected() { broadcastRejected++; }
inline void recordBeaconRejected() { beaconRejected++; }

/// True if any rejection has been recorded since boot (or since the last
/// reset()) -- i.e. there is something worth reporting.
inline bool hasEvents() {
  return encryptedCmdRejected > 0 || broadcastRejected > 0 || beaconRejected > 0;
}

/// Human-readable summary for the periodic status uplink, e.g.
/// "SECEVENTS:cmd=2,bcast=0,beacon=1". Safe to send in the clear -- these
/// are aggregate counts, not the rejected data itself.
inline std::string summary() {
  char buf[64];
  snprintf(buf, sizeof(buf), "SECEVENTS:cmd=%lu,bcast=%lu,beacon=%lu",
           (unsigned long)encryptedCmdRejected,
           (unsigned long)broadcastRejected,
           (unsigned long)beaconRejected);
  return std::string(buf);
}

/// Clears all counters back to zero, e.g. after a successful report.
inline void reset() {
  encryptedCmdRejected = 0;
  broadcastRejected = 0;
  beaconRejected = 0;
}

} // namespace securityevents

#endif  // SECURITYEVENTCOUNTERS_H_
