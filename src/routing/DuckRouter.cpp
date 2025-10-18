#include "DuckRouter.h"
std::list<Neighbor> DuckRouter::getRoutingTable() {
    std::list<Neighbor> sortedList;
    for (const auto& pair : routingTable) {
    sortedList.push_back(pair.second);
    }
    return sortedList;
};

void DuckRouter::insertIntoRoutingTable(Duid deviceID, SignalScore signalInfo, long lastSeen) {
    Neighbor record(duckutils::toString(deviceID), signalInfo.signalScore, signalInfo.snr, signalInfo.rssi, lastSeen);
    routingTable.insert(std::make_pair(signalInfo.signalScore, record));
};

void DuckRouter::CullRoutingTable(size_t maxSize) {
    std::size_t size = routingTable.size();
    while (size > maxSize) {
        auto it = std::prev(routingTable.end(),2);
        routingTable.erase(it);
        size = routingTable.size(); // Update size after erasure
    }
}

BloomFilter& DuckRouter::getFilter(){
    return filter;
}