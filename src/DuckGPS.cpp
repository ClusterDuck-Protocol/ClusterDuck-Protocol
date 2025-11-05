//
// Created by brent on 7/7/2023.
//

#include "DuckGPS.h"
#include "Adafruit_UBX.h"
void DuckGPS::setup() {
    // Configure GNSS settings: Enable GPS, disable others
    Adafruit_UBX ubx(GPSSerial);
    UBXSendStatus status = ubx.sendMessageWithAck(UBXMessageClass::UBX_CLASS_CFG, UBXCfgMessageId::UBX_CFG_GNSS,
                                                  ubx_cfg_gnss.data(), message_GNSS.size(), 1000);
    if (status != UBX_SEND_SUCCESS)
        logdbg_ln("Failed to configure GNSS settings");
    // Set update rate to 1Hz
    status = ubx.sendMessageWithAck(UBXMessageClass::UBX_CLASS_CFG, UBXCfgMessageId::UBX_CFG_RATE,
                                    message_1HZ.data(), message_1HZ.size(), 1000);
    if (status != UBX_SEND_SUCCESS)
        logdbg_ln("Failed to set update rate");
    // Configure navigation settings
    status = ubx.sendMessageWithAck(UBXMessageClass::UBX_CLASS_CFG, UBXCfgMessageId::UBX_CFG_NAVX5,
                                    message_NAVX5.data(), message_NAVX5.size(), 1000);
    if (status != UBX_SEND_SUCCESS)
        logdbg_ln("Failed to configure navigation settings");
    // Enable Jamming resistance
    status = ubx.sendMessageWithAck(UBXMessageClass::UBX_CLASS_CFG, 0x39,
                                    message_JAM.data(), message_JAM.size(), 1000);
    if (status != UBX_SEND_SUCCESS)
        logdbg_ln("Failed to enable jamming resistance");
    // Disable unnecessary NMEA sentences
    const std::array<std::pair<std::array<uint8_t,8>*, std::string>,6> disable_msgs = {
            std::make_pair(&message_GGL, "disable GGL"),
            std::make_pair(&message_GSV, "disable GSV"),
            std::make_pair(&message_VTG, "disable VTG"),
            std::make_pair(&message_GGA, "enable GGA"),
            std::make_pair(&message_GSA, "enable GSA"),
            std::make_pair(&message_RMC, "enable RMC")
    };

    for (const auto p : disable_msgs) {
        auto msg = p.first;
        status = ubx.sendMessageWithAck(UBXMessageClass::UBX_CLASS_CFG, UBXCfgMessageId::UBX_CFG_MSG,
                                        msg->data(), msg->size(), 1000);
        if (status != UBX_SEND_SUCCESS) {
            std::string err = std::string("Failed to ").append(p.second);
            logdbg_ln(err.c_str());
        }
    }
}
void DuckGPS::readData(unsigned long ms) {
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    do
    {
        if (GPSSerial.available())
            gps.encode(GPSSerial.read());
    } while (ms > std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - start).count());
    printData();
}

std::time_t DuckGPS::tmConvert_t(int YYYY, uint8_t MM, uint8_t DD, uint8_t hh, uint8_t mm, uint8_t ss)
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
    logdbg_ln("--- GPS ---");
    logdbg_ln(std::string("Latitude  : "+std::to_string(lat())).c_str());
    logdbg_ln(std::string("Longitude : "+std::to_string(lng())).c_str());
    logdbg_ln(std::string("Altitude : "+std::to_string(altitude(AltitudeUnit::meter))).append("M").c_str());
    logdbg_ln(std::string("Satellites : "+std::to_string(satellites())).c_str());
    logdbg_ln(std::string("Raw Date : "+std::to_string(gps.date.value())).c_str());
    logdbg_ln(std::string("Epoch : "+std::to_string(epoch())).c_str());
    logdbg_ln(std::string("Raw Date : "+std::to_string(speed(SpeedUnit::kmph))).append("KPH").c_str());
    logdbg_ln("**********************");
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