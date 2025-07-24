#include "DuckRouter.h"

std::list<DuckRecord> DuckRouter::getRoutingTable() {
    std::list<DuckRecord> sortedList;
    for (const auto& pair : routingTable) {
    sortedList.push_back(pair.second);
    }
    return sortedList;
};

void DuckRouter::insertIntoRoutingTable(std::string deviceID, long routingScore, long lastSeen, float snr, float rssi) {
    DuckRecord record(std::move(deviceID), routingScore, lastSeen, snr, rssi);
    routingTable.insert(std::make_pair(routingScore, record));
};