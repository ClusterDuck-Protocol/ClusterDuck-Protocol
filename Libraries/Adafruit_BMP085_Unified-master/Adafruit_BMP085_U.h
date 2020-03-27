/***************************************************************************
  This is a library for the BMP085 pressure sensor

  Designed specifically to work with the Adafruit BMP085 or BMP180 Breakout 
  ----> http://www.adafruit.com/products/391
  ----> http://www.adafruit.com/products/1603

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#ifndef __BMP085_H__
#define __BMP085_H__

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Adafruit_Sensor.h>

#ifdef __AVR_ATtiny85__
 #include "TinyWireM.h"
 #define Wire TinyWireM
#else
 #include <Wire.h>
#endif

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
    #define BMP085_ADDRESS                (0x77)
/*=========================================================================*/

/*=========================================================================
    REGISTERS
    -----------------------------------------------------------------------*/
    enum
    {
      BMP085_REGISTER_CAL_AC1            = 0xAA,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_AC2            = 0xAC,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_AC3            = 0xAE,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_AC4            = 0xB0,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_AC5            = 0xB2,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_AC6            = 0xB4,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_B1             = 0xB6,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_B2             = 0xB8,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_MB             = 0xBA,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_MC             = 0xBC,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CAL_MD             = 0xBE,  // R   Calibration data (16 bits)
      BMP085_REGISTER_CHIPID             = 0xD0,
      BMP085_REGISTER_VERSION            = 0xD1,
      BMP085_REGISTER_SOFTRESET          = 0xE0,
      BMP085_REGISTER_CONTROL            = 0xF4,
      BMP085_REGISTER_TEMPDATA           = 0xF6,
      BMP085_REGISTER_PRESSUREDATA       = 0xF6,
      BMP085_REGISTER_READTEMPCMD        = 0x2E,
      BMP085_REGISTER_READPRESSURECMD    = 0x34
    };
/*=========================================================================*/

/*=========================================================================
    MODE SETTINGS
    -----------------------------------------------------------------------*/
    typedef enum
    {
      BMP085_MODE_ULTRALOWPOWER          = 0,
      BMP085_MODE_STANDARD               = 1,
      BMP085_MODE_HIGHRES                = 2,
      BMP085_MODE_ULTRAHIGHRES           = 3
    } bmp085_mode_t;
/*=========================================================================*/

/*=========================================================================
    CALIBRATION DATA
    -----------------------------------------------------------------------*/
    typedef struct
    {
      int16_t  ac1;
      int16_t  ac2;
      int16_t  ac3;
      uint16_t ac4;
      uint16_t ac5;
      uint16_t ac6;
      int16_t  b1;
      int16_t  b2;
      int16_t  mb;
      int16_t  mc;
      int16_t  md;
    } bmp085_calib_data;
/*=========================================================================*/

class Adafruit_BMP085_Unified : public Adafruit_Sensor
{
  public:
    Adafruit_BMP085_Unified(int32_t sensorID = -1);
  
    bool  begin(bmp085_mode_t mode = BMP085_MODE_ULTRAHIGHRES);
    void  getTemperature(float *temp);
    void  getPressure(float *pressure);
    float pressureToAltitude(float seaLevel, float atmospheric);
    float seaLevelForAltitude(float altitude, float atmospheric);
    // Note that the next two functions are just for compatibility with old
    // code that passed the temperature as a third parameter.  A newer
    // calculation is used which does not need temperature.
    float pressureToAltitude(float seaLevel, float atmospheric, float temp);
    float seaLevelForAltitude(float altitude, float atmospheric, float temp);
    bool  getEvent(sensors_event_t*);
    void  getSensor(sensor_t*);

  private:
    int32_t computeB5(int32_t ut);
    int32_t _sensorID;
};

#endif
