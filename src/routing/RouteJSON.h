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
            // DUIDs are hash-derived (SHA256(pubkey) truncated) and thus
            // arbitrary binary, not printable ASCII -- duckutils::toString()
            // returns a literal "ERROR: Non-printable character" string for
            // any non-printable byte, which is virtually always true for a
            // real DUID. Hex-encode instead so the value round-trips intact.
            json["origin"] = duckutils::arrayToHexString(sourceDevice);
            json["destination"] = duckutils::arrayToHexString(targetDevice);
            json["path"].as<ArduinoJson::JsonArray>();

            std::string log;
            serializeJson(json, log);
            loginfo_ln("RouteDoc: %s", log.c_str());
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
            logdbg_ln("Built RouteJSON from packet data: %s",json.as<std::string>().c_str());
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

            std::string log;
            serializeJson(json, log);

            return json.as<std::string>();
        }
        Duid getOrigin(){
            // Decode via the bounds-safe hex decoder rather than a raw
            // std::copy of an arbitrary-length JSON string into a fixed
            // 8-byte array -- the latter overflows originDuid if `origin`
            // is ever longer than DUID_LENGTH (e.g. a legacy/corrupt
            // "ERROR: Non-printable character" value from an older build).
            return duckutils::hexStringToArray<uint8_t, DUID_LENGTH>(origin);
        }
        Duid getDestination(){
            return duckutils::hexStringToArray<uint8_t, DUID_LENGTH>(destination);
        }

        /**
         * @brief add a duck node to the path to route the request path
         *
         * @param deviceId of the duck node being added
         * @return the newly modified Arduino JSON document
         */
        std::string addToPath(Duid deviceId){
            objPath.push_back(duckutils::arrayToHexString(deviceId));
            json["path"].to<ArduinoJson::JsonArray>(); //.to erases content of the field in the doc, but .as does not modify the doc at all.
            updateJsonPath(); //so we will manually copy the local obj path to the doc
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RREQ: %s", log.c_str());
#endif
            return json.as<std::string>();
            //add rssi snr
        }

        std::optional<Duid> getlastInPath(){
            if(objPath.size() > 0){
                auto last = objPath[objPath.size()-1];
                Duid lastDuid = duckutils::hexStringToArray<uint8_t, DUID_LENGTH>(last);

                std::string log;
                serializeJson(json, log);
                logdbg_ln("RREQ: %s", log.c_str());

                return lastDuid;

            } else{
                logdbg_ln("RREQ path empty, filling with self");
                return std::nullopt;
            }
        }

    /**
     * @brief pop the last duck node from the route response path
     *
     * @param deviceId of the duck node to be removed
     * @return the newly modified Arduino JSON document
     */
    std::string popFromPath(){
        objPath.pop_back();
        updateJsonPath();
        
        std::string log;
        serializeJson(json, log);
        logdbg_ln("Packet: %s", log.c_str());

        return json.as<std::string>();
    }

  private:
        ArduinoJson::JsonDocument json;
        std::vector<std::string> objPath;
        std::string origin;
        std::string destination;

        void updateJsonPath(){
            JsonArray path = json["path"].to<JsonArray>();
            path.clear();

            for (const auto& s : objPath) {
                path.add(s);
            }
        }
  };

  #endif