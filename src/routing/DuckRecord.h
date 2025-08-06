/**
 * @file DuckLoRa.h
 * @brief This file is internal to CDP and provides direct routing
 *functionality and a routing table
 * @version
 * @date 2025-7-24
 *
 * @copyright
 */
#ifndef DUCKRECORD_H_
#define DUCKRECORD_H_

class DuckRecord {
    public:
      //DuckRecord() : routingScore(0), lastSeen(0), snr(0), rssi(0) {}
      DuckRecord(std::string devId, long routingScore, long lastSeen, float snr, float rssi) :
        DeviceId(std::move(devId)), routingScore(routingScore), lastSeen(lastSeen), snr(snr), rssi(rssi) {}
  
      std::string getDeviceId() { return DeviceId; }
      long getRoutingScore() const { return routingScore; }
      long getLastSeen() { return lastSeen; }
      long getSnr() { return snr; }
      long getRssi() { return rssi; }
  private:
      std::string DeviceId;
      long routingScore, lastSeen;
      float snr, rssi;
  };

  #endif