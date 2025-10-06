#include "DuckRouter.h"

std::list<DuckRecord> DuckRouter::getRoutingTable() {
    std::list<DuckRecord> sortedList;
    for (const auto& pair : routingTable) {
    sortedList.push_back(pair.second);
    }
    return sortedList;
};

void DuckRouter::insertIntoRoutingTable(std::string deviceID, float routingScore, long lastSeen, float snr, float rssi) {
    DuckRecord record(std::move(deviceID), routingScore, snr, rssi, lastSeen);
    routingTable.insert(std::make_pair(routingScore, record));
};

BloomFilter& DuckRouter::getFilter(){
    return filter;
}