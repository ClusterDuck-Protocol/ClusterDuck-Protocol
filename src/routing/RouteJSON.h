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
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RouteDoc: %s", log.c_str());
#endif
        }

        //Create JSON from rxPacket
        /**
         * @brief Construct a new Route JSON object from received packet data
         *
         * @param packetData the received packet data as a byte vector
         */
        RouteJSON(std::vector<uint8_t> packetData) {
            deserializeJson(json, packetData);
            path = json["path"].to<ArduinoJson::JsonArray>();
        }

        std::string asString(){
            return json.as<std::string>();
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
        /**
         * @brief get the destination device DUID from the route response
         *
         * @return the destination device DUID as a string
         */
        Duid getDestination(){
            Duid destinationDuid;
            auto destination = json["destination"].as<std::string>();
            std::copy(destination.begin(), destination.end(),destinationDuid.begin());

            return destinationDuid;
        }

        Duid getlastInPath(){
            Duid lastDuid;
            auto last = path[path.size() - 1].as<std::string>();
            std::copy(last.begin(), last.end(),lastDuid.begin());

            return lastDuid;
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
#ifdef CDP_LOG_DEBUG
                    logdbg_ln("Removing element from path: %s", v.as<std::string>().c_str());
#endif
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
  };

  #endif