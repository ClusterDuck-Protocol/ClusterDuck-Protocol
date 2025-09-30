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
            //do we want to convert to hex?
            json["origin"] = duckutils::convertToHex(sourceDevice.data(), sourceDevice.size());
            json["destination"] = duckutils::convertToHex(targetDevice.data(), targetDevice.size());
            json["path"].as<ArduinoJson::JsonArray>();
#ifdef CDP_LOG_DEBUG
            std::string log;
            serializeJson(json, log);
            logdbg_ln("RREQ: %s", log.c_str());
#endif
            // path = json.createNestedArray("path");
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

        /**
         * @brief add a duck node to the path to route the request path
         *
         * @param deviceId of the duck node being added
         * @return the newly modified Arduino JSON document
         */
        std::string addToPath(Duid deviceId){
            
             json["path"].to<ArduinoJson::JsonArray>()
                     .add(duckutils::convertToHex(deviceId.data(), deviceId.size()));
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
         * @brief pop the last duck node from the route response path
         *
         * @param deviceId of the duck node to be removed
         * @return the newly modified Arduino JSON document
         */
        ArduinoJson::JsonDocument removeFromPath(Duid deviceId){
            // ArduinoJson::JsonArray path = rreq["path"].to<ArduinoJson::JsonArray>();
            //delete object for current duid
            for (ArduinoJson::JsonVariant v : path) {
                if (v.as<std::string>() == duckutils::convertToHex(deviceId.data(), deviceId.size())) {
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
            return json;
        }

  private:
        ArduinoJson::JsonDocument json;
        ArduinoJson::JsonArray path;
  };

  #endif