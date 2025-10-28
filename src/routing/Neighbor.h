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
      Neighbor(std::string devId, Duid nextHop, SignalScore signalInfo, unsigned long lastSeen) :
        DeviceId(std::move(devId)), routingScore(signalInfo.signalScore), lastSeen(lastSeen), snr(signalInfo.snr), rssi(signalInfo.rssi) {
        // How to handle multiple next hops?
      }
        bool operator>(const Neighbor& other) const {
            return this->routingScore > other.routingScore;
        }
  
      std::string getDeviceId() const { return DeviceId; }
      long getRoutingScore() const { return routingScore; }
      long getLastSeen() const { return lastSeen; }
      long getSnr() { return snr; }
      long getRssi() { return rssi; }
  private:
      std::string DeviceId;
      long lastSeen;
      float snr, rssi, routingScore;
  };
    
  #endif