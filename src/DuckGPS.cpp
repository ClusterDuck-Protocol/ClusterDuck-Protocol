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
std::time_t DuckGPS::epoch(){
    return tmConvert_t(
            gps.date.year(),
            gps.date.month(),
            gps.date.day(),
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second());
}
void DuckGPS::printData(){
    // Printing the GPS data
    Serial.println("--- GPS ---");
    Serial.print("Latitude  : ");
    Serial.println(gps.location.lat());
    Serial.print("Longitude : ");
    Serial.println(gps.location.lng());
    Serial.print("Altitude  : ");
    Serial.print(altitude(AltitudeUnit::meter));
    Serial.println("M");
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    Serial.print("Raw Date  : ");
    Serial.println(gps.date.value());
    Serial.print("Epoch     : ");
    Serial.println(epoch());
    Serial.print("Speed     : ");
    Serial.println(speed(SpeedUnit::kmph));
    Serial.println("**********************");
}

double DuckGPS::altitude(AltitudeUnit u){
    switch(u){
        case AltitudeUnit::meter: return gps.altitude.meters();
        case AltitudeUnit::feet: return gps.altitude.feet();
        case AltitudeUnit::miles: return gps.altitude.miles();
        case AltitudeUnit::kilo: return gps.altitude.kilometers();
        default: return -1.0;
    }
}

double DuckGPS::speed(SpeedUnit u){
    switch(u){
        case SpeedUnit::kmph: return gps.speed.kmph();
        case SpeedUnit::mph: return gps.speed.mph();
        case SpeedUnit::mps: return gps.speed.mps();
        case SpeedUnit::knots: return gps.speed.knots();
        default: return -1.0;
    }
}

uint32_t DuckGPS::satellites() {
    return gps.satellites.value();
}