//
// Created by brent on 7/7/2023.
//

#ifndef CLUSTERDUCK_PROTOCOL_DUCKGPS_H
#define CLUSTERDUCK_PROTOCOL_DUCKGPS_H
#include "DuckLogger.h"
#include <include/cdpcfg.h>
#include <TinyGPS++.h>
#include <ctime>
#include <memory>

class DuckGPS {
public:
#if defined(CDPCFG_GPS_RX) && defined(CDPCFG_GPS_TX)
    DuckGPS() : GPSSerial(1) {
        GPSSerial.begin(9600, SERIAL_8N1, CDPCFG_GPS_RX, CDPCFG_GPS_TX);
    };
#endif
    DuckGPS(int Rx, int Tx) : GPSSerial(1) {
        GPSSerial.begin(9600, SERIAL_8N1, Rx, Tx);


    };
    void setup(){
        memset(&info, 0, sizeof(struct GnssModelInfo));
        uint8_t buffer[768] = {0};
        delay(100);
        GPSSerial.write("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
        delay(20);

        clearBuffer();
        int msglen = 0;
        if (strncmp(info.hwVersion, "00040007", 8) !=
            0) { // The original ublox 6 is GPS only and doesn't support the UBX-CFG-GNSS message
            if (strncmp(info.hwVersion, "00070000", 8) == 0) { // Max7 seems to only support GPS *or* GLONASS
                logdbg("Setting GPS+SBAS\n");
                msglen = makeUBXPacket(0x06, 0x3e, sizeof(_message_GNSS_7), _message_GNSS_7);
                GPSSerial.write(UBXscratch, msglen);
            } else {
                msglen = makeUBXPacket(0x06, 0x3e, sizeof(_message_GNSS), _message_GNSS);
                GPSSerial.write(UBXscratch, msglen);
            }

            if (getACK(0x06, 0x3e, 800) == GNSS_RESPONSE_NAK) {
                // It's not critical if the module doesn't acknowledge this configuration.
                loginfo("Unable to reconfigure GNSS - defaults maintained. Is this module GPS-only?\n");
            } else {
                if (strncmp(info.hwVersion, "00070000", 8) == 0) {
                    loginfo("GNSS configured for GPS+SBAS. Pause for 0.75s before sending next command.\n");
                } else {
                    loginfo("GNSS configured for GPS+SBAS+GLONASS. Pause for 0.75s before sending next command.\n");
                }
                // Documentation say, we need wait atleast 0.5s after reconfiguration of GNSS module, before sending next
                // commands

            }
            delay(850);

            msglen = makeUBXPacket(0x06, 0x39, sizeof(_message_JAM), _message_JAM);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x39, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to enable interference resistance.\n");
            }

            msglen = makeUBXPacket(0x06, 0x23, sizeof(_message_NAVX5), _message_NAVX5);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x23, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to configure extra settings.\n");
            }

            // ublox-M10S can be compatible with UBLOX traditional protocol, so the following sentence settings are also valid

            msglen = makeUBXPacket(0x06, 0x08, sizeof(_message_1HZ), _message_1HZ);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x08, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to set GPS update rate.\n");
            }
//
            msglen = makeUBXPacket(0x06, 0x01, sizeof(_message_GGL), _message_GGL);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x01, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to disable NMEA GGL.\n");
            }

            msglen = makeUBXPacket(0x06, 0x01, sizeof(_message_GSA), _message_GSA);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x01, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to Enable NMEA GSA.\n");
            }

            msglen = makeUBXPacket(0x06, 0x01, sizeof(_message_GSV), _message_GSV);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x01, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to disable NMEA GSV.\n");
            }

            msglen = makeUBXPacket(0x06, 0x01, sizeof(_message_VTG), _message_VTG);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x01, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to disable NMEA VTG.\n");
            }

            msglen = makeUBXPacket(0x06, 0x01, sizeof(_message_RMC), _message_RMC);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x01, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to enable NMEA RMC.\n");
            }

            msglen = makeUBXPacket(0x06, 0x01, sizeof(_message_GGA), _message_GGA);
            GPSSerial.write(UBXscratch, msglen);
            if (getACK(0x06, 0x01, 300) != GNSS_RESPONSE_OK) {
                logwarn_f("Unable to enable NMEA GGA.\n");
            }
        }
    }
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
    uint8_t makeUBXPacket(uint8_t class_id, uint8_t msg_id, uint8_t payload_size, const uint8_t *msg);

private:
    TinyGPSPlus gps;
    HardwareSerial GPSSerial;
    uint8_t UBXscratch[250] = {0};
    void clearBuffer()
    {
        int x = GPSSerial.available();
        while (x--)
            GPSSerial.read();
    }
    struct GnssModelInfo {
        char swVersion[30];
        char hwVersion[10];
        uint8_t extensionNo;
        char extension[10][30];
    } info;
    typedef enum
    {
        SFE_UBLOX_PACKET_VALIDITY_NOT_VALID,
        SFE_UBLOX_PACKET_VALIDITY_VALID,
        SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED,
        SFE_UBLOX_PACKET_NOTACKNOWLEDGED // This indicates that we received a NACK
    } sfe_ublox_packet_validity_e;
    typedef enum {
        GNSS_RESPONSE_NONE,
        GNSS_RESPONSE_NAK,
        GNSS_RESPONSE_FRAME_ERRORS,
        GNSS_RESPONSE_OK,
    } GPS_RESPONSE;
    GPS_RESPONSE getACK(uint8_t class_id, uint8_t msg_id, uint32_t waitMillis);
    struct ubxPacket
    {
        uint8_t cls;
        uint8_t id;
        uint16_t len;          // Length of the payload. Does not include cls, id, or checksum bytes
        uint16_t counter;      // Keeps track of number of overall bytes received. Some responses are larger than 255 bytes.
        uint16_t startingSpot; // The counter value needed to go past before we begin recording into payload array
        uint8_t *payload;      // We will allocate RAM for the payload if/when needed.
        uint8_t checksumA;     // Given to us from module. Checked against the rolling calculated A/B checksums.
        uint8_t checksumB;
        sfe_ublox_packet_validity_e valid;           // Goes from NOT_DEFINED to VALID or NOT_VALID when checksum is checked
        sfe_ublox_packet_validity_e classAndIDmatch; // Goes from NOT_DEFINED to VALID or NOT_VALID when the Class and ID match the requestedClass and requestedID
    };

    const uint8_t _message_GPSGLONASSGAL[68] = {// GPS + GALILEO + GLONASS wo / SBAS

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
    uint8_t _message_GNSS_7[20] = {
            0x00, // msgVer (0 for this version)
            0x00, // numTrkChHw (max number of hardware channels, read only, so it's always 0)
            0xff, // numTrkChUse (max number of channels to use, 0xff = max available)
            0x02, // numConfigBlocks (number of GNSS systems), most modules support maximum 3 GNSS systems
            // GNSS config format: gnssId, resTrkCh, maxTrkCh, reserved1, flags
            0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x00, 0x01, // GPS
            0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x01  // SBAS
    };
    uint8_t _message_GNSS[28] = {
            0x00, // msgVer (0 for this version)
            0x00, // numTrkChHw (max number of hardware channels, read only, so it's always 0)
            0xff, // numTrkChUse (max number of channels to use, 0xff = max available)
            0x03, // numConfigBlocks (number of GNSS systems), most modules support maximum 3 GNSS systems
            // GNSS config format: gnssId, resTrkCh, maxTrkCh, reserved1, flags
            0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01, // GPS
            0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, // SBAS
            0x06, 0x08, 0x0e, 0x00, 0x01, 0x00, 0x01, 0x01  // GLONASS
    };
    const uint8_t _message_JAM[8] = {
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
    const uint8_t _message_NAVX5[40] = {
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
    const uint8_t _message_1HZ[6] = {
            0xE8, 0x03, // Measurement Rate (1000ms for 1Hz)
            0x01, 0x00, // Navigation rate, always 1 in GPS mode
            0x01, 0x00, // Time reference
    };
    const uint8_t _message_GGL[8] =  {
            0xF0, 0x01,            // NMEA ID for GLL
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x00,                  // Disable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    const uint8_t _message_GSA[8] = {
            0xF0, 0x02,            // NMEA ID for GSA
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x01,                  // Enable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    const uint8_t _message_GSV[8] = {
        0xF0, 0x03,            // NMEA ID for GSV
                0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
                0x00,                  // Disable
                0x01, 0x01, 0x01, 0x01 // Reserved
    };
    const uint8_t _message_VTG[8] = {
            0xF0, 0x05,            // NMEA ID for VTG
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x00,                  // Disable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    const uint8_t _message_RMC[8] = {
            0xF0, 0x04,            // NMEA ID for RMC
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x01,                  // Enable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };
    const uint8_t _message_GGA[8] = {
            0xF0, 0x00,            // NMEA ID for GGA
            0x01,                  // I/O Target 0=I/O, 1=UART1, 2=UART2, 3=USB, 4=SPI
            0x01,                  // Enable
            0x01, 0x01, 0x01, 0x01 // Reserved
    };

};


#endif //CLUSTERDUCK_PROTOCOL_DUCKGPS_H
