// 
// 
// 

#include "WaveshareSharpDustSensor.h"

int WaveshareSharpDustSensor::Filter(int m)
{
	static int flag_first = 0, _buff[10], sum;
	const int _buff_max = 10;
	int i;

	if (flag_first == 0)
	{
		flag_first = 1;

		for (i = 0, sum = 0; i < _buff_max; i++)
		{
			_buff[i] = m;
			sum += _buff[i];
		}
		return m;
	}
	else
	{
		sum -= _buff[0];
		for (i = 0; i < (_buff_max - 1); i++)
		{
			_buff[i] = _buff[i + 1];
		}
		_buff[9] = m;
		sum += _buff[9];

		i = sum / 10.0;
		return i;
	}
}

float WaveshareSharpDustSensor::Conversion(int rawVoltageADC)
{
	int adcvalue = rawVoltageADC;
	/*
	covert voltage (mv)
	*/
	voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;

	/*
	voltage to density
	*/
	if (voltage >= NO_DUST_VOLTAGE)
	{
		voltage -= NO_DUST_VOLTAGE;

		density = voltage * COV_RATIO;
	}
	else
		density = 0;
	return density;
}

WaveshareSharpDustSensor::WaveshareSharpDustSensor()
{
}
