#include "DuckRouter.h"

void DuckRouter::insertIntoRoutingTable(Duid deviceID, Duid nextHop, SignalScore signalInfo) {

    Neighbor neighborRecord(deviceID, nextHop, signalInfo, millis());
    auto index = routingTable.find(duckutils::toString(deviceID));
    if (index == routingTable.end()) {
        std::list<Neighbor> neighborList;
        neighborList.push_back(neighborRecord);
        routingTable.insert(std::make_pair(neighborRecord.getDeviceId(), neighborList));
    } else {
        // Update existing record
        index->second.remove_if([neighborRecord](const Neighbor& n) {
            return n.getLastSeen() < neighborRecord.getLastSeen() && n.getDeviceId() == neighborRecord.getDeviceId();
        });
        index->second.push_back(neighborRecord);
    }
};

std::optional<Duid> DuckRouter::getBestNextHop(Duid targetDeviceId){
    //check if nextHop = the duid of the last duid in path/last duid that relayed to the current duck so that it doesn't transmit back the way it came from
    auto nextHopRecord = routingTable.find(duckutils::toString(targetDeviceId));
    if (nextHopRecord == routingTable.end()) {
        return std::nullopt; // No entry found
        loginfo_ln("=========================== NO ENTRY IN ROUTING TABLE FOUND +++++++++++++++++++++++++");
    }
    loginfo_ln("=========================== ENTRY FOUND !!!!!!!!!!! +++++++++++++++++++++++++");
    nextHopRecord->second.sort(std::greater<>());
    std::string nextHopStr = nextHopRecord->second.front().getDeviceId();
    Duid nextHopId;
    loginfo_ln("                       ENTRY DUID IS : %s", nextHopStr.c_str());
    std::copy(nextHopStr.begin(), nextHopStr.end(),nextHopId.begin());

    // if(nextHop.ttl > 0){
              //send a new rreq
    // }
    return nextHopId;

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