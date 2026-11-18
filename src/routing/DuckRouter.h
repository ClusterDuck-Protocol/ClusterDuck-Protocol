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
#include <unordered_map>
#include <list>
#include <optional>
#include "../include/cdpcfg.h"
#include "bloomfilter.h"
#include "Neighbor.h"
enum class NetworkState {SEARCHING, PUBLIC, DISCONNECTED};

class DuckRouter {
    public:
        DuckRouter() = default;
        ~DuckRouter() = default;
        BloomFilter& getFilter();
        NetworkState getNetworkState(){ return networkState; };

                /**
         * @brief Record that @p deviceID can be reached through @p nextHop.
         *
         * Replaces any earlier entry for the same destination and next hop, so
         * the newest signal reading always wins, better or worse. An entry
         * where @p deviceID equals @p nextHop is also kept as a measured direct
         * link (not for the PapaDuck or broadcast addresses). Readings that
         * aren't finite numbers are ignored. Ends with cullRoutingTable(), so
         * the candidates for each destination stay sorted best first.
         *
         * @param deviceID the destination device ID
         * @param nextHop the neighbour to send through to reach @p deviceID
         * @param signalInfo RSSI, SNR and the signal score of the packet this
         *        route was learned from; candidates are ranked by the score
         * @param heardAt millis() when that packet was received
         */
        void insertIntoRoutingTable(Duid deviceID, Duid nextHop, SignalScore signalInfo,
                                    uint32_t heardAt = millis());

        /**
         * @brief Get the next hop with the best signal score for a destination.
         *
         * Calls cullRoutingTable() first, so expired entries are gone and every
         * destination's candidates are sorted, best first.
         *
         * @param targetDeviceId the destination device ID
         * @return the next hop of the best candidate, or std::nullopt if there
         *         is no fresh route to @p targetDeviceId
         */
        std::optional<Duid> getBestNextHop(Duid targetDeviceId);
        std::optional<uint32_t> getRouteAge(Duid targetDeviceId);
        size_t directNeighborCount();
        uint32_t transitions = 0;
        uint32_t neighborEvictions = 0;
        // observed, included and millis() of the last signal serialization.
        std::optional<std::array<uint32_t, 3>> signalSnapshot;

        /**
         * @brief NetworkState if the Duck joins or disconnects from a CDP network
         * @param newState The new NetworkState to join
         */
        void setNetworkState(NetworkState newState){
            if (networkState != newState) {
                NetworkState oldState = networkState;
                networkTransition(oldState, newState);
                if (networkState != oldState) ++transitions;
            }
        }

                /**
         * @brief Cull the routing table to a maximum size and remove expired routes. Default is 3 entries.
         * @param maxSize the maximum size of neighbor entries per destination target
         */
        void cullRoutingTable(size_t maxSize = 3);

        /**
         * @brief retrieve all neighbor entries for a given destination target
         * @param targetDuid destination duid
         * @returns the list of all best-next-hops for the destination id in string format, to be sent as packet data.
         */
        std::optional<std::string> getEntriesFor(Duid targetDuid, Duid thisDuck);

    private:
        std::unordered_map<std::string, std::list<Neighbor>> routingTable;
        // A destination changing next hop must never overwrite a measured link.
        std::unordered_map<std::string, Neighbor> directNeighbors;
        BloomFilter filter;
        NetworkState networkState = NetworkState::SEARCHING;
        unsigned long ROUTE_TTL = CDPCFG_ROUTE_TTL_MS;

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
