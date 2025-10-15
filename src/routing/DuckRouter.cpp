#include "DuckRouter.h"
#include <radio/Signal.h>

std::list<DuckRecord> DuckRouter::getRoutingTable() {
    std::list<DuckRecord> sortedList;
    for (const auto& pair : routingTable) {
    sortedList.push_back(pair.second);
    }
    return sortedList;
};

void DuckRouter::insertIntoRoutingTable(std::string deviceID,long lastSeen, Signal signalInfo) {
    DuckRecord record(std::move(deviceID), signalInfo.signalScore, signalInfo.snr, signalInfo.rssi, lastSeen);
    routingTable.insert(std::make_pair(signalInfo.signalScore, record));
};

BloomFilter& DuckRouter::getFilter(){
    return filter;
}