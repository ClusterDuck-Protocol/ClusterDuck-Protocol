//
// Created by brent on 7/7/2023.
//

#include "include/DuckGPS.h"

void DuckGPS::readData(unsigned long ms) {
    unsigned long start = millis();
    do
    {
        while (GPSSerial.available())
            gps.encode(GPSSerial.read());
    } while (millis() - start < ms);
}

std::time_t DuckGPS::tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
    std::tm tmSet{};
    tmSet.tm_year = YYYY - 1900;
    tmSet.tm_mon = MM - 1;
    tmSet.tm_mday = DD;
    tmSet.tm_hour = hh;
    tmSet.tm_min = mm;
    tmSet.tm_sec = ss;
    std::time_t t = std::mktime(&tmSet);
    return mktime(std::gmtime(&t));
}

void DuckGPS::printData(){
    time_t t= tmConvert_t(
            gps.date.year(),
            gps.date.month(),
            gps.date.day(),
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second());
    // Printing the GPS data
    Serial.println("--- GPS ---");
    Serial.print("Latitude  : ");
    Serial.println(gps.location.lat(), 5);
    Serial.print("Longitude : ");
    Serial.println(gps.location.lng(), 4);
    Serial.print("Altitude  : ");
    Serial.print(gps.altitude.meters());
    Serial.println("M");
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    Serial.print("Raw Date  : ");
    Serial.println(gps.date.value());
    Serial.print("Epoch     : ");
    Serial.println(t);
    Serial.print("Speed     : ");
    Serial.println(gps.speed.kmph());
    Serial.println("**********************");
}