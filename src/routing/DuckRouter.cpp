#include "DuckRouter.h"
#include <ArduinoJson.h>

void DuckRouter::insertIntoRoutingTable(Duid deviceID, Duid nextHop, SignalScore signalInfo) {

    Neighbor neighborRecord(deviceID, nextHop, signalInfo, millis());
    auto index = routingTable.find(duckutils::duidAsString(deviceID));
    if (index == routingTable.end()) { //need to make sure we aren't adding Link1276->Link1276 to Mama1262
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
    auto nextHopRecord = routingTable.find(duckutils::duidAsString(targetDeviceId));
    if (nextHopRecord == routingTable.end()) {
        return std::nullopt; // No entry found
    }
    nextHopRecord->second.sort(std::greater<>()); 
    std::string nextHopStr = nextHopRecord->second.front().getDeviceId();
    Duid nextHopId;
    std::copy(nextHopStr.begin(), nextHopStr.end(),nextHopId.begin());

    if(nextHopRecord->second.front().getLastSeen() >= millis()- ROUTE_TTL){ 
        Serial.printf(
            "millis=%lu lastSeen=%lu ttl=%lu threshold=%lu\n",
            millis(),
            nextHopRecord->second.front().getLastSeen(),
            ROUTE_TTL,
            millis() - ROUTE_TTL
        );
        Serial.println(nextHopStr.c_str());
              loginfo_ln("[ROUTER] route ttl expired");
              routingTable.erase(nextHopStr);
              return std::nullopt;
    }
    return nextHopId;

};

void DuckRouter::cullRoutingTable(size_t maxSize) {
    //iterate through and remove all expired ttl routes based off of the route ttl expiry above

    auto neighborIndex = routingTable.begin();
    while(neighborIndex != routingTable.end()) {
        auto& neighborList = neighborIndex->second;
        
        auto entry = neighborList.begin();
        while(entry != neighborList.end()) {
            if (entry->getLastSeen() >= millis()- ROUTE_TTL) {
                loginfo_ln("[ROUTER] culling route with ttl expired");
                entry = neighborList.erase(entry);
            } else {
                ++entry;
            }
        }
        
        if (neighborList.empty()) {
            neighborIndex = routingTable.erase(neighborIndex);
        } else {
            ++neighborIndex;
        }
        //check to make sure entries per destination doesn't exceed max
        std::size_t size = routingTable.size();
        while (size > maxSize) {
            auto it = std::prev(routingTable.end(),1); // Get iterator to the last element
            loginfo_ln("[ROUTER] culling route that exceeded max entries");
            routingTable.erase(it);
            size = routingTable.size(); // Update size after erasure
        }
    }
};

std::optional<std::string> DuckRouter::getEntriesFor(Duid targetDuid, Duid thisDuck){
    auto target = routingTable.find(duckutils::duidAsString(targetDuid));
    JsonDocument doc;

    if (target == routingTable.end()) {
        return std::nullopt;
    }

    doc["s"] = duckutils::hexToString(duckutils::duidAsString(thisDuck));
    JsonArray neighborsArr = doc["n"].to<JsonArray>();

    auto entry = target->second.begin();
    while(entry != target->second.end()){
        JsonArray node = neighborsArr.createNestedArray();
        node.add(duckutils::hexToString(duckutils::duidAsString(entry->getDuid())));
        node.add(entry->getRssi());
        node.add(entry->getSnr());
        entry++;
    }

    std::string jsonString;
    serializeJson(doc, jsonString);

    return jsonString;
}

BloomFilter& DuckRouter::getFilter(){
    return filter; //just call the bloomfilter function here?
};