/**
 * @file RouteJSON.h
 * @brief This file is internal to CDP and provides route JSON construction
 *  and manipulation
 * @version
 * @date 2025-7-24
 *
 * @copyright
 */
#ifndef RouteJSON_H
#define RouteJSON_H

#include <ArduinoJson.h>
#include <optional>
#include <algorithm>
#include "../utils/DuckUtils.h"
#include "../CdpPacket.h"

class RouteJSON {
    public:
        /**
         * @brief Construct a new Route JSON object. Duid is a typedef of std::array<uint8_t,8>
         *
         * @param targetDevice the destination device DUID
         * @param sourceDevice the source device DUID
         */
        RouteJSON(Duid targetDevice, Duid sourceDevice) {
            origin = toText(sourceDevice);
            destination = toText(targetDevice);
            json["origin"] = origin;
            json["destination"] = destination;
            json["path"].to<ArduinoJson::JsonArray>();
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RouteDoc: %s", log.c_str());
#endif
        }

        /**
         * @brief Parse route JSON out of received packet data.
         *
         * This is the only way to build a RouteJSON from over-the-air data. It
         * returns nullopt and logs the specific reason when the payload is
         * malformed, so a caller cannot end up holding a half-parsed document --
         * there is no validity flag left to forget to check.
         *
         * @param packetData the received packet data as a byte vector
         * @returns the parsed document, or nullopt if the payload is not usable
         */
        static std::optional<RouteJSON> fromPacketData(const std::vector<uint8_t>& packetData) {
            RouteJSON doc;
            if (!doc.parsePacketData(packetData)) {
                return std::nullopt;
            }
            return doc;
        }

        std::string asString(){
            std::string out;
            serializeJson(json, out);
            return out;
        }

        std::string convertReqToRep(){
            std::string oldOrigin = origin;
            //update rreq to rrep
            origin = destination;
            destination = oldOrigin;
            json["origin"] = origin;
            json["destination"] = destination;

#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RREP: %s", log.c_str());
#endif
            return asString();
        }
        Duid getOrigin(){ return fromText(origin); }
        Duid getDestination(){ return fromText(destination); }

        /**
         * @brief Write a DUID as route JSON text.
         *
         * Broadcast (raw 0xFF) and the PapaDuck (raw 0x00) are not valid JSON
         * strings, so they travel as "*" and "11111111" (PAPADUCK_ALIAS_DUID).
         */
        static std::string toText(const Duid& id) {
            if (id == BROADCAST_DUID) return "*";
            if (id == PAPADUCK_DUID) return duckutils::duidToString(PAPADUCK_ALIAS_DUID);
            return duckutils::duidToString(id);
        }

        /// Read route JSON text back into a DUID; the inverse of toText().
        static Duid fromText(const std::string& text) {
            if (text == "*") return BROADCAST_DUID;
            Duid id{};
            if (text.size() == DUID_LENGTH) std::copy(text.begin(), text.end(), id.begin());
            return id == PAPADUCK_ALIAS_DUID ? PAPADUCK_DUID : id;
        }

        /**
         * @brief add a duck node to the path to route the request path
         *
         * @param deviceId of the duck node being added
         * @return the newly modified Arduino JSON document
         */
        std::string addToPath(Duid deviceId){
            objPath.push_back(toText(deviceId));
            json["path"].to<ArduinoJson::JsonArray>(); //.to erases content of the field in the doc, but .as does not modify the doc at all.
            updateJsonPath(); //so we will manually copy the local obj path to the doc
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RREQ: %s", log.c_str());
#endif
            return asString();
            //add rssi snr
        }

        std::optional<Duid> getlastInPath(){
            if(objPath.size() > 0){
                const std::string& last = objPath[objPath.size()-1];
                if (last.size() != DUID_LENGTH) {
                    logdbg_ln("RREQ path entry has invalid length, ignoring");
                    return std::nullopt;
                }
                return fromText(last);

            } else{
                logdbg_ln("RREQ path empty, filling with self");
                return std::nullopt;
            }
        }

    /**
     * @brief pop the last duck node from the route response path
     *
     * @return the newly modified Arduino JSON document
     */
    std::string popFromPath(){
        if (objPath.empty()) {
            //pop_back on an empty vector is UB
            logdbg_ln("popFromPath: path already empty, nothing to pop");
            return asString();
        }
        objPath.pop_back();
        updateJsonPath();
#ifdef CDP_LOG_DEBUG
        std::string log;
        serializeJson(json, log);
        logdbg_ln("Packet: %s", log.c_str());
#endif
        return asString();
    }

        size_t pathSize() const { return objPath.size(); }
        bool contains(Duid id) const {
            return std::find(objPath.begin(), objPath.end(), toText(id)) != objPath.end();
        }
        bool startsAt(Duid id) const {
            return !objPath.empty() && objPath.front() == toText(id);
        }
        std::optional<Duid> getVia() const {
            const std::string via = json["via"].as<std::string>();
            if (via.size() != DUID_LENGTH) return std::nullopt;
            return fromText(via);
        }
        void setVia(Duid id) { json["via"] = toText(id); }
        RouteJSON responseFrom(Duid responder, Muid requestId) const {
            RouteJSON response(*this);
            response.convertReqToRep();
            response.origin = toText(responder);
            response.json["origin"] = response.origin;
            response.json["q"] = duckutils::arrayToHexString(requestId);
            return response;
        }

  private:
        /// Only fromPacketData() may build an unpopulated document.
        RouteJSON() = default;

        /**
         * @brief Populate this document from received packet data.
         *
         * Every field that is later copied into a fixed-size Duid is length
         * checked here, so getOrigin()/getDestination()/getlastInPath() cannot
         * over-run their destination buffers with attacker-shaped JSON.
         *
         * @returns false if the payload is malformed; the reason is logged.
         */
        bool parsePacketData(const std::vector<uint8_t>& packetData) {
            if (packetData.empty() || packetData.size() > MAX_DATA_LENGTH) return false;
            std::string packetStr(packetData.begin(), packetData.end());
            DeserializationError error = deserializeJson(json, packetStr);
            if (error) {
                logerr_ln("RouteJSON deserialization failed: %s", error.c_str());
                return false;
            }

            const char* originPtr = json["origin"].as<const char*>();
            const char* destinationPtr = json["destination"].as<const char*>();
            if (originPtr == nullptr || destinationPtr == nullptr) {
                logerr_ln("RouteJSON missing origin/destination");
                return false;
            }
            origin = json["origin"].as<std::string>();
            destination = json["destination"].as<std::string>();
            if (origin.size() != DUID_LENGTH ||
                (destination.size() != DUID_LENGTH && destination != "*")) {
                logerr_ln("RouteJSON origin/destination length invalid (%d/%d)",
                          (int)origin.size(), (int)destination.size());
                return false;
            }

            if (!printableDuid(origin) ||
                (destination != "*" && destination != duckutils::duidToString(BROADCAST_DUID) && !printableDuid(destination))) return false;
            if (!json["path"].is<JsonArray>() || json["path"].size() > MAX_HOPS + 1) return false;
            if (json["via"].isNull() == false && !printableDuid(json["via"].as<std::string>())) return false;
            if (json["q"].isNull() == false && json["q"].as<std::string>().size() != MUID_LENGTH * 2) return false;
            for (JsonVariant value : json["path"].as<JsonArray>()) {
                std::string entry = value.as<std::string>();
                if (!printableDuid(entry) ||
                    std::find(objPath.begin(), objPath.end(), entry) != objPath.end()) {
                    logerr_ln("RouteJSON path entry length invalid (%d)", (int)entry.size());
                    return false;
                }
                objPath.push_back(entry);
            }

#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("Built RouteJSON from packet data: %s", log.c_str());
#endif
            return true;
        }

        ArduinoJson::JsonDocument json;
        static bool printableDuid(const std::string& value) {
            return value.size() == DUID_LENGTH && std::all_of(value.begin(), value.end(),
                [](unsigned char c) { return c >= 0x20 && c <= 0x7e; });
        }
        std::vector<std::string> objPath;
        std::string origin;
        std::string destination;

        /**
         * @brief update the JSON document's path field with the current objPath vector. The JsonArray path variable is
         * a reference that points to a JsonDocument, so there is only need to manipulate the underlying memory.
         * see https://arduinojson.org/v7/api/jsonarray/
         */
        void updateJsonPath(){
            JsonArray path = json["path"].to<JsonArray>();
            path.clear(); //clear the path array in the doc so we can update it with the new path vector

            for (const auto& s : objPath) {
                if (!path.add(s))
                    logerr_ln("Failed to add %s to JSON path array; No more memory in JSON Document", s.c_str());
            }
        }
  };

  #endif
