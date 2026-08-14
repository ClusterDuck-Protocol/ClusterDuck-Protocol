/**
 * @file Neighbor.h
 * @brief This file is internal to CDP and sorts nearest neighbors
 * on a route path
 * @version
 * @date 2025-7-24
 *
 * @copyright
 */
#ifndef NEIGHBOR_H
#define NEIGHBOR_H
#include "SignalScore.h"
#include <list>

class Neighbor {
    public:
      Neighbor(Duid devId, Duid nextHop, SignalScore signalInfo, unsigned long lastSeen) :
        DeviceId(devId), routingScore(signalInfo.signalScore), lastSeen(lastSeen), snr(signalInfo.snr), rssi(signalInfo.rssi) {
        // How to handle multiple next hops?
      }
        bool operator>(const Neighbor& other) const {
            return this->routingScore > other.routingScore;
        }
  
      [[nodiscard]] std::string getDeviceId() const {
        // NOTE: intentionally NOT duckutils::toString() -- that helper
        // collapses any non-printable byte to the literal string
        // "ERROR: Non-printable character", which would collide every
        // hash-derived (non-printable) DUID into a single routing table
        // bucket. std::string can safely hold arbitrary bytes, including
        // embedded NULs, so this raw construction round-trips correctly.
        return std::string(DeviceId.begin(), DeviceId.end());
      }
      long getRoutingScore() const { return routingScore; }
      unsigned long getLastSeen() const { return lastSeen; }
      long getSnr() { return snr; }
      long getRssi() { return rssi; }
  private:
      Duid DeviceId;
      unsigned long lastSeen;
      float snr, rssi, routingScore;
  };
    
  #endif