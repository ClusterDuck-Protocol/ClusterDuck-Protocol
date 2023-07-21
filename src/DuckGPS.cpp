//
// Created by brent on 7/7/2023.
//

#include "DuckGPS.h"
#include "DuckLogger.h"

void DuckGPS::readData(unsigned long ms) {
    unsigned long start = millis();
    do
    {
        while (GPSSerial.available())
            gps.encode(GPSSerial.read());
    } while (millis() - start < ms);
    printData();
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
    logdbg("--- GPS ---");
    logdbg(std::string("Latitude  : "+std::to_string(lat())).c_str());
    logdbg(std::string("Longitude : "+std::to_string(lng())).c_str());
    logdbg(std::string("Altitude : "+std::to_string(altitude(AltitudeUnit::meter))).append("M").c_str());
    logdbg(std::string("Satellites : "+std::to_string(satellites())).c_str());
    logdbg(std::string("Raw Date : "+std::to_string(gps.date.value())).c_str());
    logdbg(std::string("Epoch : "+std::to_string(epoch())).c_str());
    logdbg(std::string("Raw Date : "+std::to_string(speed(SpeedUnit::kmph))).append("KPH").c_str());
    logdbg("**********************");
}


double DuckGPS::lat() {
    return gps.location.lat();
}
double DuckGPS::lng() {
    return gps.location.lng();
}
std::string DuckGPS::geoJsonPoint(){
    std::string geojsonpoint = "{\"type\": \"Point\", \"coordinates\": [";
    return geojsonpoint
        .append(std::to_string(lat()))
        .append(",")
        .append(std::to_string(lng())
        .append("]}"));
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