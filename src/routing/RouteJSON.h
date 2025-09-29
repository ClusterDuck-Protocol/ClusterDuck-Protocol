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
        //Create new JSON for rreq
        RouteJSON(Duid targetDevice, Duid sourceDevice) {
            json["origin"] = duckutils::convertToHex(sourceDevice.data(), sourceDevice.size());
            json["destination"] = duckutils::convertToHex(targetDevice.data(), targetDevice.size());
            // path = json.createNestedArray("path");
        }

        //Create JSON from rxPacket
        RouteJSON(std::vector<u_int8_t> packetData): {
            JsonDocument rreq = deserializeJson(rreq, packetData);
            origin = //string to duid
            path = rreq["path"].to<ArduinoJson::JsonArray>();
        }

        /**
         * @brief add a duck node to the path to route the request path
         *
         * @param deviceId of the duck node being added
         * @return the newly modified Arduino JSON document
         */
        std::string addToPath(Duid deviceId){
            
            // ArduinoJson::JsonArray path = rreq["path"].to<ArduinoJson::JsonArray>();
            // path.add(currentDevice);
            // rreq["path"] = path;
            // serializeJson(rreqDoc, strRREQ);
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

        }

  private:
        ArduinoJson::JsonDocument json;
        ArduinoJson::JsonArray path;
  };

  #endif