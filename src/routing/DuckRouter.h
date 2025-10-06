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
#include "DuckRecord.h"

class DuckRouter {
    public:
        DuckRouter() = default;;
        ~DuckRouter() = default;;
        BloomFilter& getFilter();
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
        void insertIntoRoutingTable(std::string deviceID, float routingScore, long lastSeen, float snr, float rssi);
        
        // //put this on Router
        // void updateRoutingTable(){
        //   Serial.println("routing table creation")
        // }
  
    private:
        std::multimap<float,DuckRecord,std::greater<>> routingTable;
        BloomFilter filter;
        
};
  #endif