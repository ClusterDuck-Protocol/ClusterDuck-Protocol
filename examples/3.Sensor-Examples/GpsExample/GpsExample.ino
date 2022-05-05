
/**
 * @file GpsExample.ino
 * @brief Uses the built in Mama Duck and TinyGPS++
 * 
 * This example is a Custom MamaDuck with a GPS receiver, and it is periodically sending a message with the location.
 * Recommended Hardware: TTGO T-Beam
 */

#include <string>
#include <arduino-timer.h>
#include <MamaDuck.h>

#include <TinyGPS++.h>
#include "utilities.h"

// create a built-in mama duck
MamaDuck duck;

// create a GPS handler
TinyGPSPlus gpsHandler;

// create a timer with default settings
auto timer = timer_create_default();

// variables for handling current location
bool lastPosValid = false;
double lastPosLat;
double lastPosLon;

// constants for orchestrating location updates
const double GPS_DISTANCE_THRESHOLD = 10;         // in meters
const double GPS_UPDATE_INTERVAL = 1000 * 10;     // in milliseconds

void setup() {
    Serial.begin(115200);
    initGPS();
    delay(2000);
    std::string deviceId("MAMAGPS1");
    std::vector<byte> devId;
    devId.insert(devId.end(), deviceId.begin(), deviceId.end());
    duck.setupWithDefaults(devId);

    duck.onCaptivePortalFormSubmission(appendGPSMessage);

    timer.every(GPS_UPDATE_INTERVAL, updateGPS);
    
    Serial.print("GPS duck init done.");    
}

void appendGPSMessage(String* message) {
    if(gpsHandler.location.isValid()) {
      *message += getGPSMessage();
    } else {
      Serial.println("appendGPSMessage(): GPS location invalid");
    }
}

void loop() {
    duck.run();
    timer.tick();
}

String getGPSMessage() {
    char buf[20];
    String gpsMessage = String("");
    String comma = String(", ");

    TinyGPSDate date = gpsHandler.date;
    if(date.isValid()) {        
        sprintf(buf, "%04d-%02d-%02d", date.year(), date.month(), date.day());
        gpsMessage += String(buf);
    }
    gpsMessage += comma;
    TinyGPSTime time = gpsHandler.time;
    if(time.isValid()) {
        String sep = String(":");
        sprintf(buf, "%02d:%02d:%02d", time.hour(), time.minute(), time.second());
        gpsMessage += String(buf);
    }
    gpsMessage += comma;
    gpsMessage += String(gpsHandler.location.lat(), 5) + comma;
    gpsMessage += String(gpsHandler.location.lng(), 5);
    if(gpsHandler.altitude.isValid()) {
        gpsMessage += comma + String(gpsHandler.altitude.meters());
    }

    return gpsMessage;
}

bool updateGPS(void*) {
    Serial.print("GPS UPDATE: ");

    while (Serial1.available() > 0)
      gpsHandler.encode(Serial1.read());

    TinyGPSLocation location = gpsHandler.location;
    if (!location.isValid()) {
      Serial.println("Failed (no GPS data available)");
      return true;
    }

    if (lastPosValid) {
      // Only send update if we've passed a certain distance to the last sent position
      double distance = TinyGPSPlus::distanceBetween(lastPosLat, lastPosLon, location.lat(), location.lng());
      if (distance < GPS_DISTANCE_THRESHOLD) {
        Serial.println(String("Not sent - distance too short: ") + String(distance, 6));
        return true;
      }
    }

    String gpsMessage = getGPSMessage();

    Serial.println("Sending...");       
    Serial.println(gpsMessage);    
    
    lastPosLat = location.lat();
    lastPosLon = location.lng();
    lastPosValid = true;    

    std::vector<byte> muid;
    duck.sendData(topics::location, gpsMessage, ZERO_DUID, &muid);    
    
    return true;
}

