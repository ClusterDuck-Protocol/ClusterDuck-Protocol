/*********************************************************************************************************
*
* File                : DustSensor
* Hardware Environment:
* Build Environment   : Arduino
* Version             : V1.0.5-r2
* By                  : WaveShare
*
*                                  (c) Copyright 2005-2011, WaveShare
*                                       http://www.waveshare.net
*                                       http://www.waveshare.com
*                                          All Rights Reserved
*
*********************************************************************************************************/
#include "WaveshareSharpDustSensor.h"



/*
I/O define
*/
const int iled = 4;                                            //drive the led of sensor
const int vout = 0;                                            //analog input

															   /*
															   variable
															   */

int   adcvalue;

/*
private function
*/

WaveshareSharpDustSensor ds;

void setup(void)
{
	pinMode(iled, OUTPUT);
	digitalWrite(iled, LOW);                                     //iled default closed

	Serial.begin(9600);                                         //send and receive at 9600 baud
	//Serial.print("*********************************** WaveShare ***********************************\n");
	
}

void loop(void)
{
	/*
	get adcvalue
	*/
	digitalWrite(iled, HIGH);
	delayMicroseconds(280);
	adcvalue = analogRead(vout);
	digitalWrite(iled, LOW);

	adcvalue = ds.Filter(adcvalue);

	float density = ds.Conversion(adcvalue);

	/*
	display the result
	*/
	
	Serial.print("The current dust concentration is: ");
	Serial.print(density);
	Serial.print(" ug/m3\n");

	delay(1000);
}






















