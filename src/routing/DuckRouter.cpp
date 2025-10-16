#include "DuckRouter.h"

std::list<Neighbor> DuckRouter::getRoutingTable() {
    std::list<Neighbor> sortedList;
    for (const auto& pair : routingTable) {
    sortedList.push_back(pair.second);
    }
    return sortedList;
};

void DuckRouter::insertIntoRoutingTable(std::string deviceID,long lastSeen, Signal signalInfo) {
    Neighbor record(std::move(deviceID), signalInfo.signalScore, signalInfo.snr, signalInfo.rssi, lastSeen);
    routingTable.insert(std::make_pair(signalInfo.signalScore, record));
};

BloomFilter& DuckRouter::getFilter(){
    return filter;
}