#include "DuckRouter.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <cmath>
#include <vector>

void DuckRouter::insertIntoRoutingTable(Duid deviceID, Duid nextHop, SignalScore signal, uint32_t heardAt) {
    if (!std::isfinite(signal.rssi) || !std::isfinite(signal.snr) || !std::isfinite(signal.signalScore)) return;
    Neighbor entry(deviceID, nextHop, signal, heardAt);
    const auto key = duckutils::duidToString(deviceID);
    auto& candidates = routingTable[key];
    if (candidates.empty()) {
        logdbg_ln("Adding new neighbor record to routing table for deviceID: %s", key);
    } else {
        logdbg_ln("Updating existing neighbor record in routing table for deviceID: %s", key);
    }
    // Replace only this next hop, even within the same millisecond.
    candidates.remove_if([&](const Neighbor& n) { return n.getNextHop() == nextHop; });
    candidates.push_back(entry);
    if (deviceID == nextHop && deviceID != PAPADUCK_DUID && deviceID != BROADCAST_DUID)
        directNeighbors.insert_or_assign(key, entry);
    cullRoutingTable();
}

std::optional<Duid> DuckRouter::getBestNextHop(Duid target) {
    cullRoutingTable();
    const auto key = duckutils::duidToString(target);
    logdbg_ln("Searching for best next hop for target device ID: %s", key);
    const auto it = routingTable.find(key);
    if (it == routingTable.end() || it->second.empty()) {
        logdbg_ln("No routing entry found for target device ID: %s", key);
        return std::nullopt;
    }
    return it->second.front().getNextHop();
}

std::optional<uint32_t> DuckRouter::getRouteAge(Duid target) {
    if (!getBestNextHop(target)) return std::nullopt;
    return uint32_t(millis() - routingTable.at(duckutils::duidToString(target)).front().getLastSeen());
}

void DuckRouter::cullRoutingTable(size_t maxSize) {
    const uint32_t now = millis();
    for (auto it = routingTable.begin(); it != routingTable.end();) {
        auto& candidates = it->second;
        candidates.remove_if([&](const Neighbor& n) { return uint32_t(now - n.getLastSeen()) >= ROUTE_TTL; });
        candidates.sort(std::greater<>());
        while (candidates.size() > maxSize) candidates.pop_back();
        if (candidates.empty()) it = routingTable.erase(it);
        else ++it;
    }
    for (auto it = directNeighbors.begin(); it != directNeighbors.end();) {
        if (uint32_t(now - it->second.getLastSeen()) >= ROUTE_TTL) it = directNeighbors.erase(it);
        else ++it;
    }
    while (directNeighbors.size() > CDPCFG_MAX_DIRECT_NEIGHBORS) {
        auto oldest = std::max_element(directNeighbors.begin(), directNeighbors.end(), [&](const auto& a, const auto& b) {
            return uint32_t(now - a.second.getLastSeen()) < uint32_t(now - b.second.getLastSeen());
        });
        directNeighbors.erase(oldest);
        ++neighborEvictions;
    }
    while (routingTable.size() > CDPCFG_MAX_ROUTE_DESTINATIONS) {
        auto oldest = std::max_element(routingTable.begin(), routingTable.end(), [&](const auto& a, const auto& b) {
            return uint32_t(now - a.second.front().getLastSeen()) < uint32_t(now - b.second.front().getLastSeen());
        });
        routingTable.erase(oldest);
    }
}

size_t DuckRouter::directNeighborCount() {
    cullRoutingTable();
    return directNeighbors.size();
}

std::optional<std::string> DuckRouter::getEntriesFor(Duid, Duid self) {
    cullRoutingTable();
    std::vector<Neighbor> direct;
    for (const auto& pair : directNeighbors) direct.push_back(pair.second);
    std::sort(direct.begin(), direct.end(), [](const Neighbor& a, const Neighbor& b) {
        return a.getRoutingScore() == b.getRoutingScore() ? a.getDeviceId() < b.getDeviceId() : a > b;
    });
    JsonDocument doc;
    doc["s"] = duckutils::duidToString(self);
    auto array = doc["n"].to<JsonArray>();
    doc["o"] = direct.size();
    doc["i"] = 0;
    doc["tr"] = !direct.empty();
    doc["a"] = nullptr;
    doc["v"] = 2;
    uint32_t oldestMs = 0;
    size_t included = 0;
    for (const auto& peer : direct) {
        if (included >= CDPCFG_SIGNAL_MAX_NEIGHBORS) break;
        auto row = array.add<JsonArray>();
        row.add(peer.getDeviceId());
        // Preserve the normalized RSSI/SNR contract already consumed by DMS.
        row.add(std::round(peer.getRssi() * 10000.0f) / 10000.0f);
        row.add(std::round(peer.getSnr() * 10000.0f) / 10000.0f);
        const uint32_t age = std::max(oldestMs, uint32_t(millis() - peer.getLastSeen()));
        doc["i"] = included + 1;
        doc["tr"] = included + 1 < direct.size();
        doc["a"] = age / 1000;
        if (measureJson(doc) > MAX_DATA_LENGTH) {
            array.remove(array.size() - 1);
            break;
        }
        ++included;
        oldestMs = age;
    }
    doc["i"] = included;
    doc["tr"] = included < direct.size();
    if (included) doc["a"] = oldestMs / 1000;
    else doc["a"] = nullptr;
    std::string result;
    signalSnapshot = std::array<uint32_t, 3>{uint32_t(direct.size()), uint32_t(included), uint32_t(millis())};
    serializeJson(doc, result);
    loginfo_ln("[ROUTER] signal neighbors observed=%u included=%u bytes=%u",
               (unsigned)direct.size(), (unsigned)included, (unsigned)result.size());
    return result;
}

BloomFilter& DuckRouter::getFilter() { return filter; }
