/**
 * @file DuckLoRa.h
 * @brief This file is internal to CDP and provides direct routing
 *functionality and a routing table
 * @version
 * @date 2025-7-24
 *
 * @copyright
 */
#ifndef DUCKROUTER_H_
#define DUCKROUTER_H_

#include <map>
#include <list>
#include "bloomfilter.h"

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
      BloomFilter filter = BloomFilter();
  };

class DuckRouter {
    public:
        DuckRouter();
        ~DuckRouter();
    protected:
        /**
         * @brief Sort the routing table using the customGreater comparator
         */
        std::list<DuckRecord> getRoutingTable();

        //  void sortRoutingTable() {
        //    getRoutingTable().sort([](const DuckRecord& lhs, const DuckRecord& rhs){
        //        return lhs.getRoutingScore() > rhs.getRoutingScore();
        //    });
        //  }
        /**
         * @brief Insert a new record into the routing table
         *
         * @param deviceID the device ID
         * @param routingScore the routing score
         * @param lastSeen the last seen timestamp
         * @param snr the signal to noise ratio
         * @param rssi the received signal strength indicator
         */
        void insertIntoRoutingTable(std::string deviceID, long routingScore, long lastSeen, float snr, float rssi);
        
        // //put this on Router
        // void updateRoutingTable(){
        //   Serial.println("routing table creation")
        // }
  
    private:
        std::multimap<long,DuckRecord,std::greater<long>> routingTable;
        BloomFilter filter;
        
};
  #endif