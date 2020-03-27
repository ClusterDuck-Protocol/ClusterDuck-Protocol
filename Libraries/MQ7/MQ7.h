/*
Library for CO (Carbon Monoxide Gas) ppm value
by using MQ7 gas sensor.

Datasheet : https://www.sparkfun.com/datasheets/Sensors/Biometric/MQ-7.pdf
*/

#ifndef MQ7_h
#define MQ7_h

/*
The coefficients are estimated from the sensitivity characteristics graph
of the MQ7 sensor for CO (Carbon Monoxide) gas by using Correlation function.

Explanation :
	The graph in the datasheet is represented with the function
	f(x) = a * (x ^ b).
	where
		f(x) = ppm
		x = Rs/R0
	The values were mapped with this function to determine the coefficients a and b.
*/

#define coefficient_A 19.32
#define coefficient_B -0.64

//Load resistance 10 Kohms on the sensor potentiometer
#define R_Load 10.0

class MQ7 {
	private:
		uint8_t analogPin;
		float v_in;
		float voltageConversion(int);
	public:
		MQ7(uint8_t, float);
		float getPPM();
		float getSensorResistance();
		float getRatio();
};

#endif
