//
// Created by brent on 7/7/2023.
//

#ifndef CLUSTERDUCK_PROTOCOL_DUCKGPS_H
#define CLUSTERDUCK_PROTOCOL_DUCKGPS_H
#include "utils/DuckLogger.h"
#include "include/cdpcfg.h"
#include "TinyGPS++.h"
#include "Adafruit_UBloxDDC.h"
#include "Adafruit_uBlox_typedef.h"
#include <ctime>
#include <memory>
#include <array>
#include <chrono>

class DuckGPS {
public:
#if defined(CDPCFG_GPS_RX) && defined(CDPCFG_GPS_TX)
    DuckGPS() : GPSSerial(1){
        GPSSerial.begin(9600, SERIAL_8N1, CDPCFG_GPS_RX, CDPCFG_GPS_TX);
    };
#endif
    DuckGPS(int Rx, int Tx) : GPSSerial(1) {
        GPSSerial.begin(9600, SERIAL_8N1, Rx, Tx);


    };
    /**
     * @brief initializes GPS chip with recommended settings
     */
    void setup();
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
    std::time_t tmConvert_t(int YYYY, uint8_t MM, uint8_t DD, uint8_t hh, uint8_t mm, uint8_t ss);

private:
    TinyGPSPlus gps;
    HardwareSerial GPSSerial;

    void clearBuffer()
    {
        int x = GPSSerial.available();
        while (x--)
            GPSSerial.read();
    }

    std::array<uint8_t,58> ubx_cfg_gnss = {
            0x00,0x08,0x10,0x00,0x01,0x00,0x01,0x01, // GPS, Min/Max Channel Resources, ENABLED, L1, BIT24 (per uCenter/Query, cut-n-paste of hex frame)
            0x01,0x01,0x03,0x00,0x01,0x00,0x01,0x01, // SBAS
            0x02,0x04,0x08,0x00,0x01,0x00,0x01,0x01, // GALILEO
            0x03,0x08,0x10,0x00,0x00,0x00,0x01,0x01, // BEIDOU (DISABLED)
            0x04,0x00,0x08,0x00,0x00,0x00,0x01,0x01, // IMES (DISABLED)
            0x05,0x00,0x03,0x00,0x01,0x00,0x01,0x01, // QZSS
            0x06,0x08,0x0E,0x00,0x01,0x00,0x01,0x01, // GLONASS
            0x30,0xAD }; // Fletcher checksum, correct for preceeding frame

    std::array<uint8_t,68> message_GPSGLONASSGAL = {// GPS + GALILEO + GLONASS wo / SBAS

            0xB5,0x62,0x06,0x3E, 0x3C, 0x00,

            0x00,0x00,0x20,0x07,

            0x00,0x00,0x10,0x00,0x01,0x00,0x01,0x01,

            0x01,0x00,0x00,0x00,0x00,0x00,0x01,0x01,

            0x02,0x00,0x08,0x00,0x01,0x00,0x01,0x01,

            0x03,0x00,0x00,0x00,0x00,0x00,0x01,0x01,

            0x04,0x00,0x00,0x00,0x00,0x00,0x01,0x01,

            0x05,0x00,0x00,0x00,0x00,0x00,0x01,0x01,

            0x06,0x00,0x10,0x00,0x01,0x00,0x01,0x01,

            0xF5,0x8A};

    std::array<uint8_t,44> message_GNSS_8 = {
            0x00,                                           // msgVer (0 for this version)
            0x00,                                           // numTrkChHw (max number of hardware channels, read only, so it's always 0)
            0xff,                                           // numTrkChUse (max number of channels to use, 0xff = max available)
            0x05,                                           // numConfigBlocks (number of GNSS systems)
            // GNSS config format: gnssId, resTrkCh, maxTrkCh, reserved1, flags
            0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01, // GPS
            0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, // SBAS
            0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, // Galileo
            0x05, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, // QZSS
            0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01  // GLONASS
    };
    std::array<uint8_t,20> message_NMEA = {
            0x00,                              // filter flags
            0x41,                              // NMEA Version
            0x00,                              // Max number of SVs to report per TaklerId
            0x02,                              // flags
            0x00, 0x00, 0x00, 0x00,            // gnssToFilter
            0x00,                              // svNumbering
            0x00,                              // mainTalkerId
            0x00,                              // gsvTalkerId
            0x01,                              // Message version
            0x00, 0x00,                        // bdsTalkerId 2 chars 0=default
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Reserved
    };
    std::array<uint8_t,28> message_GNSS = {
            0x00, // msgVer (0 for this version)
            0x00, // numTrkChHw (max number of hardware channels, read only, so it's always 0)
            0xff, // numTrkChUse (max number of channels to use, 0xff = max available)
            0x03, // numConfigBlocks (number of GNSS systems), most modules support maximum 3 GNSS systems
            // GNSS config format: gnssId, resTrkCh, maxTrkCh, reserved1, flags
            0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01, // GPS
            0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, // SBAS
            0x06, 0x08, 0x0e, 0x00, 0x01, 0x00, 0x01, 0x01  // GLONASS
    };
    std::array<uint8_t,8> message_JAM = {
            // bbThreshold (Broadband jamming detection threshold) is set to 0x3F (63 in decimal)
            // cwThreshold (CW jamming detection threshold) is set to 0x10 (16 in decimal)
            // algorithmBits (Reserved algorithm settings) is set to 0x16B156 as recommended
            // enable (Enable interference detection) is set to 1 (enabled)
            0x3F, 0x10, 0xB1, 0x56, // config: Interference config word
            // generalBits (General settings) is set to 0x31E as recommended
            // antSetting (Antenna setting, 0=unknown, 1=passive, 2=active) is set to 0 (unknown)
            // ToDo: Set to 1 (passive) or 2 (active) if known, for example from UBX-MON-HW, or from board info
            // enable2 (Set to 1 to scan auxiliary bands, u-blox 8 / u-blox M8 only, otherwise ignored) is set to 1
            // (enabled)
            0x1E, 0x03, 0x00, 0x01 // config2: Extra settings for jamming/interference monitor
    };
    std::array<uint8_t,40> message_NAVX5 = {
            0x00, 0x00, // msgVer (0 for this version)
            // minMax flag = 1: apply min/max SVs settings
            // minCno flag = 1: apply minimum C/N0 setting
            // initial3dfix flag = 0: apply initial 3D fix settings
            // aop flag = 1: apply aopCfg (useAOP flag) settings (AssistNow Autonomous)
            0x1B, 0x00, // mask1 (First parameters bitmask)
            // adr flag = 0: apply ADR sensor fusion on/off setting (useAdr flag)
            // If firmware is not ADR/UDR, enabling this flag will fail configuration
            // ToDo: check this with UBX-MON-VER
            0x00, 0x00, 0x00, 0x00,             // mask2 (Second parameters bitmask)
            0x00, 0x00,                         // Reserved
            0x03,                               // minSVs (Minimum number of satellites for navigation) = 3
            0x10,                               // maxSVs (Maximum number of satellites for navigation) = 16
            0x06,                               // minCNO (Minimum satellite signal level for navigation) = 6 dBHz
            0x00,                               // Reserved
            0x00,                               // iniFix3D (Initial fix must be 3D) = 0 (disabled)
            0x00, 0x00,                         // Reserved
            0x00,                               // ackAiding (Issue acknowledgements for assistance message input) = 0 (disabled)
            0x00, 0x00,                         // Reserved
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
            0x00,                               // Reserved
            0x01,                               // aopCfg (AssistNow Autonomous configuration) = 1 (enabled)
            0x00, 0x00,                         // Reserved
            0x00, 0x00,                         // Reserved
            0x00, 0x00, 0x00, 0x00,             // Reserved
            0x00, 0x00, 0x00,                   // Reserved
            0x01,                               // useAdr (Enable/disable ADR sensor fusion) = 1 (enabled)
    };
    std::array<uint8_t,6> message_1HZ = {
            0xE8, 0x03, // Measurement Rate (1000ms for 1Hz)
            0x01, 0x00, // Navigation rate, always 1 in GPS mode
            0x01, 0x00, // Time reference
    };
    std::array<uint8_t,8> message_GGL =  {
            0xF0, 0x01,            // NMEA ID for GLL
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x00,                  // Disable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    std::array<uint8_t,8> message_GSA = {
            0xF0, 0x02,            // NMEA ID for GSA
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x01,                  // Enable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    std::array<uint8_t,8> message_GSV = {
            0xF0, 0x03,            // NMEA ID for GSV
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x00,                  // Disable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    std::array<uint8_t,8> message_VTG = {
            0xF0, 0x05,            // NMEA ID for VTG
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x00,                  // Disable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    std::array<uint8_t,8> message_RMC = {
            0xF0, 0x04,            // NMEA ID for RMC
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x01,                  // Enable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    std::array<uint8_t,8> message_GGA = {
            0xF0, 0x00,            // NMEA ID for GGA
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x01,                  // Enable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };

};


#endif //CLUSTERDUCK_PROTOCOL_DUCKGPS_H
