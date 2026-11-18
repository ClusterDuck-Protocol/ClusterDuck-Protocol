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
        DeviceId(devId), NextHop(nextHop), routingScore(signalInfo.signalScore), lastSeen(lastSeen), snr(signalInfo.snr), rssi(signalInfo.rssi) {
      }
        bool operator>(const Neighbor& other) const {
            return this->routingScore > other.routingScore;
        }
  
      [[nodiscard]] std::string getDeviceId() const { return duckutils::duidToString(DeviceId); }
      float getRoutingScore() const { return routingScore; }
      unsigned long getLastSeen() const { return lastSeen; }
      float getSnr() const { return snr; }
      float getRssi() const { return rssi; }
      Duid getDuid() const { return DeviceId; }
      [[nodiscard]] Duid getNextHop() const { return NextHop; }

      /**
       * @brief True when this entry was learned by hearing the node itself, not
       * via a relay -- so its rssi/snr describe a real one-hop link. Entries
       * learned from an RREP carry the relay's signal, which must not be
       * reported as the destination's.
       */
      [[nodiscard]] bool isDirectNeighbor() const { return NextHop == DeviceId; }
  private:
      Duid DeviceId;
      Duid NextHop;
      unsigned long lastSeen;
      float snr, rssi, routingScore;
  };
    
  #endif
