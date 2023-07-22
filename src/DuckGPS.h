//
// Created by brent on 7/7/2023.
//

#ifndef CLUSTERDUCK_PROTOCOL_DUCKGPS_H
#define CLUSTERDUCK_PROTOCOL_DUCKGPS_H
#include <TinyGPS++.h>
#include "include/cdpcfg.h"
#include <ctime>
#include <memory>

class DuckGPS {
public:
#if defined(CDPCFG_GPS_RX) && defined(CDPCFG_GPS_TX)
    DuckGPS() : GPSSerial(1) {
        GPSSerial.begin(9600, SERIAL_8N1,CDPCFG_GPS_RX,CDPCFG_GPS_TX);
    };
#endif
    DuckGPS(int RX, int TX) : GPSSerial(1) {
        GPSSerial.begin(9600, SERIAL_8N1, RX, TX);
    };
    enum AltitudeUnit{
        meter,
        kilo,
        feet,
        miles
    };
    enum SpeedUnit{
        kmph,
        mps, // Speed in meters per second
        mph, // Speed in miles per hour
        knots
    };
    /**
   * @brief continuously reads data from GPS chip
   *
   * @param unsigned long value for number of milliseconds to read data for
   * @return none
   */
    void readData(unsigned long ms);
    double lat(), lng();
    /**
  * @brief helper method for getting [lat,lng] as a GeoJSON object.
  *
  * @param none
  * @return std::string with GeoJSON object that can be parsed by the developer
  */
    std::string geoJsonPoint();
    /**
   * @brief helper method to return time from GPS data
   *
   * @param none
   * @return std::time_t in epoch seconds
   */
    std::time_t epoch();
    /**
   * @brief helper method to return speed
   *
   * @param DuckGPS::SpeedUnit
   * @return double value for chosen speed unit
   */
    double speed(SpeedUnit u);
    /**
   * @brief helper method to return number of satellites
   *
   * @param none
   * @return uint32_t value for number of satellites
   */
    uint32_t satellites();
    /**
   * @brief helper method to return altitude
   *
   * @param DuckGPS::AltitudeUnit
   * @return double value for chosen altitude unit
   */
    double altitude(AltitudeUnit u);
protected:
    void printData();
    std::time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss);
private:
    TinyGPSPlus gps;
    HardwareSerial GPSSerial;

};


#endif //CLUSTERDUCK_PROTOCOL_DUCKGPS_H
