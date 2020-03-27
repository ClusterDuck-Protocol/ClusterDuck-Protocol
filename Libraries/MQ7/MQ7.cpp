/*
Library for CO (Carbon Monoxide Gas) ppm value
by using MQ7 gas sensor.

Datasheet : https://www.sparkfun.com/datasheets/Sensors/Biometric/MQ-7.pdf
*/

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include <avr/io.h>
    #include "WProgram.h"
#endif

#include "MQ7.h"
/*
Constructor to initialize the analog pin and input voltage to MQ7

@param pin : Input pin from MQ7 analog pin
@param v_input : Input voltage to MQ7 sensor
*/
MQ7::MQ7(uint8_t pin, float v_input){
  analogPin = pin;
  v_in = v_input;
}

/*
Function is used to return the ppm value of CO gas concentration
by using the parameter found using the function f(x) = a * ((Rs/R0) ^ b)

@return ppm value of Carbon Monoxide concentration
*/
float MQ7::getPPM(){
  return (float)(coefficient_A * pow(getRatio(), coefficient_B));
}

/*
This function returns voltage from the analog input value
Refer ADC Conversion for further reference

@param value : value from analogPin

@return voltage
*/
float MQ7::voltageConversion(int value){
  return (float) value * (v_in / 1023.0);
}

/*
This function is for the deriving the Rs/R0 to find ppm

@return The value of Rs/R_Load
*/
float MQ7::getRatio(){
  int value = analogRead(analogPin);
  float v_out = voltageConversion(value);
  return (v_in - v_out) / v_out;
}
/*
To find the sensor resistance Rs

@return Rs value
*/
float MQ7::getSensorResistance(){
  return R_Load * getRatio();
}
