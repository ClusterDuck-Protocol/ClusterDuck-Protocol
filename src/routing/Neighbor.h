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
  
      [[nodiscard]] std::string getDeviceId() const { return duckutils::hexToString(duckutils::duidAsString(DeviceId)); }
      long getRoutingScore() const { return routingScore; }
      unsigned long getLastSeen() const { return lastSeen; }
      float getSnr() { return snr; }
      float getRssi() { return rssi; }
      Duid getDuid(){  return DeviceId; }
  private:
      Duid DeviceId;
      unsigned long lastSeen;
      float snr, rssi, routingScore;
  };
    
  #endif