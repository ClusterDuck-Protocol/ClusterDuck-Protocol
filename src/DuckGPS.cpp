//
// Created by brent on 7/7/2023.
//

#include "DuckGPS.h"
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

uint8_t DuckGPS::makeUBXPacket(uint8_t class_id, uint8_t msg_id, uint8_t payload_size, const uint8_t *msg)
{
    // Construct the UBX packet
    UBXscratch[0] = 0xB5;         // header
    UBXscratch[1] = 0x62;         // header
    UBXscratch[2] = class_id;     // class
    UBXscratch[3] = msg_id;       // id
    UBXscratch[4] = payload_size; // length
    UBXscratch[5] = 0x00;

    UBXscratch[6 + payload_size] = 0x00; // CK_A
    UBXscratch[7 + payload_size] = 0x00; // CK_B

    for (int i = 0; i < payload_size; i++) {
        UBXscratch[6 + i] = pgm_read_byte(&msg[i]);
    }

    uint8_t CK_A = 0, CK_B = 0;
    int length = payload_size + 8;
    for (size_t i = 2; i < length - 2; i++) {
        CK_A = (CK_A + UBXscratch[i]) & 0xFF;
        CK_B = (CK_B + CK_A) & 0xFF;
    }

    // Place the calculated checksum values in the message
    UBXscratch[length - 2] = CK_A;
    UBXscratch[length - 1] = CK_B;
    return length;
}

DuckGPS::GPS_RESPONSE DuckGPS::getACK(uint8_t class_id, uint8_t msg_id, uint32_t waitMillis)
{
uint8_t b;
uint8_t ack = 0;
const uint8_t ackP[2] = {class_id, msg_id};
uint8_t buf[10] = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
uint32_t startTime = millis();
const char frame_errors[] = "More than 100 frame errors";
int sCounter = 0;

for (int j = 2; j < 6; j++) {
buf[8] += buf[j];
buf[9] += buf[8];
}

for (int j = 0; j < 2; j++) {
buf[6 + j] = ackP[j];
buf[8] += buf[6 + j];
buf[9] += buf[8];
}

while (millis() - startTime < waitMillis) {
if (ack > 9) {
#ifdef GPS_DEBUG
LOG_DEBUG("\n");
            LOG_INFO("Got ACK for class %02X message %02X in %d millis.\n", class_id, msg_id, millis() - startTime);
#endif
return GNSS_RESPONSE_OK; // ACK received
}
if (GPSSerial.available()) {
b = GPSSerial.read();
if (b == frame_errors[sCounter]) {
sCounter++;
if (sCounter == 26) {
return GNSS_RESPONSE_FRAME_ERRORS;
}
} else {
sCounter = 0;
}
#ifdef GPS_DEBUG
LOG_DEBUG("%02X", b);
#endif
if (b == buf[ack]) {
ack++;
} else {
if (ack == 3 && b == 0x00) { // UBX-ACK-NAK message
#ifdef GPS_DEBUG
LOG_DEBUG("\n");
#endif
logwarn("Got NAK for class %02X message %02X\n", class_id, msg_id);
return GNSS_RESPONSE_NAK; // NAK received
}
ack = 0; // Reset the acknowledgement counter
}
}
}
#ifdef GPS_DEBUG
LOG_DEBUG("\n");
    logwarn_f("No response for class %02X message %02X\n", class_id, msg_id);
#endif
return GNSS_RESPONSE_NONE; // No response received within timeout
}