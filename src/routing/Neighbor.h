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
class Neighbor {
    public:
      Neighbor(std::string devId, Duid nextHop, SignalScore signalInfo, long lastSeen) :
        DeviceId(std::move(devId)), routingScore(signalInfo.signalScore), lastSeen(lastSeen), snr(signalInfo.snr), rssi(signalInfo.rssi) {}
  
      std::string getDeviceId() { return DeviceId; }
      long getRoutingScore() const { return routingScore; }
      long getLastSeen() { return lastSeen; }
      long getSnr() { return snr; }
      long getRssi() { return rssi; }
  private:
      std::string DeviceId;
      long lastSeen;
      float snr, rssi, routingScore;
  };
    
  #endif