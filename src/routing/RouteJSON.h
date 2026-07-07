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
#include "../utils/DuckUtils.h"
#include "../CdpPacket.h"

class RouteJSON {
    public:
        /**
         * @brief Construct a new Route JSON object
         *
         * @param targetDevice the destination device DUID
         * @param sourceDevice the source device DUID
         */
        RouteJSON(Duid targetDevice, Duid sourceDevice) {
            origin = duckutils::hexToString(duckutils::duidAsString(sourceDevice));
            destination = duckutils::hexToString(duckutils::duidAsString(targetDevice));
            json["origin"] = origin;
            json["destination"] = destination;
            json["path"].as<ArduinoJson::JsonArray>();
            valid = true;

            std::string log;
            serializeJson(json, log);
            loginfo_ln("RouteDoc: %s", log.c_str());
        }

        //Create JSON from rxPacket
        /**
         * @brief Construct a new Route JSON object from received packet data
         * check isValid() before using the parsed values
         *
         * @param packetData the received packet data as a byte vector
         */
        RouteJSON(std::vector<uint8_t> packetData) {
            std::string packetStr(packetData.begin(), packetData.end());
            DeserializationError error = deserializeJson(json, packetStr);
            if (error) {
                logerr_ln("RouteJSON deserialization failed: %s", error.c_str());
                return;
            }

            const char* originPtr = json["origin"].as<const char*>();
            const char* destinationPtr = json["destination"].as<const char*>();
            if (originPtr == nullptr || destinationPtr == nullptr) {
                logerr_ln("RouteJSON missing origin/destination");
                return;
            }
            origin = originPtr;
            destination = destinationPtr;
            if (origin.size() != DUID_LENGTH || destination.size() != DUID_LENGTH) {
                logerr_ln("RouteJSON origin/destination length invalid (%d/%d)",
                          (int)origin.size(), (int)destination.size());
                return;
            }

            for (JsonVariant value : json["path"].as<JsonArray>()) {
                std::string entry = value.as<std::string>();
                if (entry.size() != DUID_LENGTH) {
                    logerr_ln("RouteJSON path entry length invalid (%d)", (int)entry.size());
                    return;
                }
                objPath.push_back(entry);
            }

            valid = true;
            const std::string serialized = asString();
            logdbg_ln("Built RouteJSON from packet data: %s", serialized.c_str());
        }

        /**
         * @brief returns false if the packet data could not be parsed
         */
        bool isValid() const { return valid; }

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

            std::string log;
            serializeJson(json, log);

            return asString();
        }
        Duid getOrigin(){
            Duid originDuid;
            originDuid.fill(0);
            if (origin.size() == DUID_LENGTH) {
                std::copy(origin.begin(), origin.end(), originDuid.begin());
            }
            return originDuid;
        }
        Duid getDestination(){
            Duid destinationDuid;
            destinationDuid.fill(0);
            if (destination.size() == DUID_LENGTH) {
                std::copy(destination.begin(), destination.end(), destinationDuid.begin());
            }
            return destinationDuid;
        }

        /**
         * @brief add a duck node to the path to route the request path
         *
         * @param deviceId of the duck node being added
         * @return the newly modified Arduino JSON document
         */
        std::string addToPath(Duid deviceId){
            objPath.push_back(duckutils::hexToString(duckutils::duidAsString(deviceId)));
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
            Duid lastDuid;
            if(objPath.size() > 0){
                const std::string& last = objPath[objPath.size()-1];
                if (last.size() != DUID_LENGTH) {
                    logdbg_ln("RREQ path entry has invalid length, ignoring");
                    return std::nullopt;
                }
                std::copy(last.begin(), last.end(), lastDuid.begin());
                return lastDuid;

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

        std::string log;
        serializeJson(json, log);
        logdbg_ln("Packet: %s", log.c_str());

        return asString();
    }

  private:
        ArduinoJson::JsonDocument json;
        std::vector<std::string> objPath;
        std::string origin;
        std::string destination;
        bool valid = false;

        void updateJsonPath(){
            JsonArray path = json["path"].to<JsonArray>();
            path.clear();

            for (const auto& s : objPath) {
                path.add(s);
            }
        }
  };

  #endif
