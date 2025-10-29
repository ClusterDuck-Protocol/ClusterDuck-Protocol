#include "DuckRouter.h"

//std::list<Neighbor> DuckRouter::getRoutingTable() {
//    std::list<Neighbor> sortedList;
//    for (const auto& pair : routingTable) {
//    sortedList.push_back(pair.second);
//    }
//    return sortedList;
//};

void DuckRouter::insertIntoRoutingTable(Duid deviceID, Duid nextHop, SignalScore signalInfo) {

    Neighbor neighborRecord(duckutils::toString(deviceID), nextHop, signalInfo, millis());
    auto it = routingTable.find(duckutils::toString(deviceID));
    if (it == routingTable.end()) {
        std::list<Neighbor> neighborList;
        neighborList.push_back(neighborRecord);
        routingTable.insert(std::make_pair(duckutils::toString(deviceID), neighborList));
    } else {
        // Update existing record
        it->second.push_back(neighborRecord);
        it->second.remove_if([neighborRecord](const Neighbor& n) {
            return n.getLastSeen() < neighborRecord.getLastSeen() && n.getDeviceId() == neighborRecord.getDeviceId();
        });
    }
};

std::optional<Duid> DuckRouter::getBestNextHop(Duid targetDeviceId){
    // I'm not sure if this is what we're looking for since we still need to handle
    // sending the very first message to the target when we don't have a known path yet.
    auto it = routingTable.find(duckutils::toString(targetDeviceId));
    if (it == routingTable.end()) {
        return std::nullopt; // No entry found
    }
    it->second.sort(std::greater<>());
    auto duid = it->second.front().getDeviceId();
    return duckutils::stringToArray<uint8_t,8>(duid);

};

void DuckRouter::CullRoutingTable(size_t maxSize) {
    std::size_t size = routingTable.size();
    while (size > maxSize) {
        auto it = std::prev(routingTable.end(),1); // Get iterator to the last element
        routingTable.erase(it);
        size = routingTable.size(); // Update size after erasure
    }
};

BloomFilter& DuckRouter::getFilter(){
    return filter; //just call the bloomfilter function here?
};