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
            json["origin"] = duckutils::toString(sourceDevice);
            json["destination"] = duckutils::toString(targetDevice);
            json["path"].as<ArduinoJson::JsonArray>();
// #ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            loginfo_ln("RouteDoc: %s", log.c_str());
// #endif
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
            path = json["path"].as<ArduinoJson::JsonArray>();
            origin = json["origin"].as<const char*>();
            destination = json["destination"].as<const char*>();
            logdbg_ln("Built RouteJSON from packet data: %s",json.as<std::string>().c_str());
        }

        std::string asString(){
            return json.as<std::string>();
        }

        void convertReqToRep(){
            std::string oldOrigin = origin;
            //update rreq to rrep
            origin = destination;
            destination = oldOrigin;

            std::string log;
            serializeJson(json, log);
            loginfo_ln(" ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ RREP Doc Updated: %s", log.c_str());
            logdbg_ln("LOG OF JSON !!!!!1: %s",json.as<std::string>().c_str());
            Serial.println("===================================================================================================");
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
            
             json["path"].to<ArduinoJson::JsonArray>()
                     .add(duckutils::toString(deviceId));
             path = json["path"];
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RREQ: %s", log.c_str());
#endif
            return json.as<std::string>();
            //add rssi snr
        }

        std::optional<Duid> getlastInPath(){
            Duid lastDuid;
            if(path.size() > 0){
                auto last = path[path.size()-1].as<std::string>();
                std::copy(last.begin(), last.end(),lastDuid.begin());

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
        std::string removeFromPath(Duid deviceId){
            //delete object for current duid
            for (ArduinoJson::JsonVariant v : path) {
                if (v.as<std::string>() == duckutils::toString(deviceId)) {

                    logdbg_ln("Removing element from path: %s", v.as<std::string>().c_str());

                    path.remove(v);
#ifdef CDP_LOG_DEBUG
                    std::string log;
                    serializeJson(json, log);
                    logdbg_ln("Packet: %s", log.c_str());
#endif
                    break;
                }
            }
            return json.as<std::string>();
        }

  private:
        ArduinoJson::JsonDocument json;
        ArduinoJson::JsonArray path;
        std::string origin;
        std::string destination;
  };

  #endif