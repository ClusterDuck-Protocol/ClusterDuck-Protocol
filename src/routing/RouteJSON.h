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
         * @brief Construct a new Route JSON object. Duid is a typedef of std::array<uint8_t,8>
         *
         * @param targetDevice the destination device DUID
         * @param sourceDevice the source device DUID
         */
        RouteJSON(Duid targetDevice, Duid sourceDevice) {
            json["origin"] = duckutils::toString(sourceDevice);
            json["destination"] = duckutils::toString(targetDevice);
            json["path"].as<ArduinoJson::JsonArray>();
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            loginfo_ln("RouteDoc: %s", log.c_str());
#endif
        }

        //Create JSON from rxPacket
        /**
         * @brief Construct a new Route JSON object from received packet data
         *
         * @param packetData the received packet data as a byte vector
         */
        RouteJSON(std::vector<uint8_t> packetData) {
            std::string packetStr(packetData.begin(), packetData.end());
            DeserializationError error = deserializeJson(json, packetStr);
            if (error) {
                logerr_ln("RouteJSON deserialization failed: %s", error.c_str());
            }
            for (JsonVariant value : json["path"].as<JsonArray>()) {
                objPath.push_back(value);  // Copy each element to myPath
            }
            origin = json["origin"].as<const char*>();
            destination = json["destination"].as<const char*>();
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("Built RouteJSON from packet data: %s",log));
#endif
        }

        std::string asString(){
            return json.as<std::string>();
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
            logdbg_ln("RREP: %s",log);
#endif
            return json.as<std::string>();
        }
        Duid getOrigin(){
            Duid originDuid;
            std::copy(origin.begin(), origin.end(), originDuid.begin());
            return originDuid;
        }
        Duid getDestination(){
            Duid destinationDuid;
            std::copy(destination.begin(), destination.end(), destinationDuid.begin());
            return destinationDuid;
        }

        /**
         * @brief add a duck node to the path to route the request path
         *
         * @param deviceId of the duck node being added
         * @return the newly modified Arduino JSON document
         */
        std::string addToPath(Duid deviceId){
            objPath.push_back(duckutils::toString(deviceId));
            json["path"].to<ArduinoJson::JsonArray>(); //.to erases content of the field in the doc, but .as does not modify the doc at all.
            updateJsonPath(); //so we will manually copy the local obj path to the doc
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RREQ: %s", log);
#endif
            return json.as<std::string>();
            //add rssi snr
        }

        std::optional<Duid> getlastInPath(){
            Duid lastDuid;
            if(objPath.size() > 0){
                auto last = objPath[objPath.size()-1];
                std::copy(last.begin(), last.end(),lastDuid.begin());
#ifdef CDP_LOG_DEBUG
                std::string log;
                serializeJson(json, log);
                logdbg_ln("RREQ: %s",log);
#endif
                return lastDuid;

            } else{
                logdbg_ln("RREQ path empty, filling with self");
                return std::nullopt;
            }
        }

    /**
     * @brief pop the last duck node from the route response path
     * @return the newly modified Arduino JSON document
     */
    std::string popFromPath(){
        objPath.pop_back();
        updateJsonPath();
#ifdef CDP_LOG_DEBUG
        std::string log;
        serializeJson(json, log);
        logdbg_ln("Packet: %s", log);
#endif
        return json.as<std::string>();
    }

  private:
        ArduinoJson::JsonDocument json;
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
                    logerr_ln("Failed to add %s to JSON path array; No more memory in JSON Document", s);
            }
        }
  };

  #endif