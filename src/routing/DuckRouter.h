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
#include "Neighbor.h"
enum class NetworkState {SEARCHING, PUBLIC, DISCONNECTED};

class DuckRouter {
    public:
        DuckRouter() = default;;
        ~DuckRouter() = default;;
        BloomFilter& getFilter();
        NetworkState getNetworkState(){ return networkState; };

                /**
         * @brief Insert a new record into the routing table
         *
         * @param deviceID the device ID
         * @param routingScore the routing score
         * @param lastSeen the last seen timestamp
         * @param snr the signal to noise ratio
         * @param rssi the received signal strength indicator
         */
        void insertIntoRoutingTable(Duid deviceID, Duid nextHop, SignalScore signalInfo);

        Duid getBestNextHop(Duid targetDeviceId);

        /**
         * @brief NetworkState if the Duck joins or disconnects from a CDP network
         * @param newState The new NetworkState to join
         */
        void setNetworkState(NetworkState newState){
            if (networkState != newState) {
                NetworkState oldState = networkState;
                networkTransition(oldState, newState);
            }
        }
    protected:
        /**
         * @brief Sort the routing table using the customGreater comparator
         */
        std::list<Neighbor> getRoutingTable();
        /**
         * @brief Cull the routing table to a maximum size. Default is 3 entries. Can be expanded for larger networks.
         * @param maxSize the maximum size of the routing table
         */
        void CullRoutingTable(size_t maxSize = 3);

        //  void sortRoutingTable() {
        //    getRoutingTable().sort([](const Neighbor& lhs, const Neighbor& rhs){
        //        return lhs.getRoutingScore() > rhs.getRoutingScore();
        //    });
        //  }
        
        // //put this on Router
        // void updateRoutingTable(){
        //   Serial.println("routing table creation")
        // }
  
    private:
        std::multimap<float,Neighbor,std::greater<>> routingTable;
        BloomFilter filter;
        NetworkState networkState = NetworkState::SEARCHING;

        /**
         * @brief NetworkState transition for NetworkState FSM
         * @param oldState NetworkState to transition out of
         * @param newState NetworkState to transition in to
         */
        void networkTransition(NetworkState oldState, NetworkState newState){
            if (oldState == NetworkState::SEARCHING && newState == NetworkState::PUBLIC) {
                loginfo_ln("[ROUTER] Public network joined.");
                networkState = newState;
            } else if (oldState == NetworkState::PUBLIC && newState == NetworkState::DISCONNECTED){
                networkState = newState;
                loginfo_ln("[ROUTER] Successfully disconnected from CDP network.");
            } else if(oldState == NetworkState::PUBLIC && newState == NetworkState::SEARCHING){
                networkState = newState;
                loginfo_ln("[ROUTER] Lost connection to CDP Network.");
            } else if(oldState == NetworkState::DISCONNECTED && newState == NetworkState::SEARCHING){
                networkState = newState;
                logdbg_ln("[ROUTER] Leaving disconnected state, looking for CDP networks.");
            } else {
                logdbg_ln("[ROUTER] Invalid network state transition!");
            }
        }
        
};
  #endif