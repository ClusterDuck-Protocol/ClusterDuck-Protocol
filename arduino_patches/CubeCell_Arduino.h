/*
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Arduino_h
#define Arduino_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib_noniso.h>
#include <string.h>
#include <math.h>
#include <ASR_Arduino.h>
#include <HardwareSerial.h>
#include <CyLib.h>
#include "board.h"
#include "timer.h"
#include "rtc-board.h"
#include "hw.h"
#include "low_power.h"
#include "asr_timer.h"
#include "binary.h"

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define LSBFIRST 0
#define MSBFIRST 1

#define  lowByte(w)   ((w) & 0xff)
#define  highByte(w)   ((w) >> 8)
#define  bitRead(value, bit)   (((value) >> (bit)) & 0x01)
#define  bitWrite(value, bit, bitvalue)   (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#define  bitSet(value, bit)   ((value) |= (1UL << (bit)))
#define  bitClear(value, bit)   ((value) &= ~(1UL << (bit)))
#define  bit(b)   (1 << (b))
#define  _BV(b)   (1UL << (b))
#define yield()

#ifdef __cplusplus
#include <algorithm>

using std::abs;
using std::max;
using std::min;
#else
#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
#define abs(x)   ((x)>0?(x):-(x))
#endif /* __cplusplus */

#define constrain(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define _min(a,b) ((a)<(b)?(a):(b))
#define _max(a,b) ((a)>(b)?(a):(b))
#define noInterrupts() CyGlobalIntDisable
#define interrupts()   CyGlobalIntEnable
#define digitalPinToInterrupt(p) ( p )

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

#define pinMode(pin,mode) PINMODE_ ## mode(pin)
#define PINMODE_ANALOG(pin) pinMode(pin,ANALOG)
#define PINMODE_INPUT(pin) pinMode(pin,INPUT)
#define PINMODE_OUTPUT_PULLUP(pin) pinMode(pin,OUTPUT_PULLUP)
#define PINMODE_OUTPUT_PULLDOWN(pin) pinMode(pin,OUTPUT_PULLDOWN)
#define PINMODE_OD_LO(pin) pinMode(pin,OD_LO)
#define PINMODE_OD_HI(pin) pinMode(pin,OD_HI)
#define PINMODE_OUTPUT(pin) pinMode(pin,OUTPUT)
#define PINMODE_OUTPUT_PULLUP_PULLDOWN(pin) pinMode(pin,OUTPUT_PULLUP_PULLDOWN)
#define PINMODE_INPUT_PULLUP(pin) ({pinMode(pin,OUTPUT_PULLUP); digitalWrite(pin,HIGH);})
#define PINMODE_INPUT_PULLDOWN(pin) ({pinMode(pin,OUTPUT_PULLDOWN); digitalWrite(pin,LOW);})

typedef bool boolean;
typedef uint8_t byte;
typedef uint16_t word;

// some libraries and sketches depend on this AVR stuff,
// assuming Arduino.h or WProgram.h automatically includes it...
//

uint16_t makeWord(uint16_t w);
uint16_t makeWord(byte h, byte l);

#define word(...) makeWord(__VA_ARGS__)

uint32_t pulseIn(uint8_t pin_name,uint8_t mode,uint32_t timeout = 1000000);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint16_t getBatteryVoltage(void);
uint8_t BoardGetBatteryLevel(void); 

long random(long howbig);
void randomSeed(unsigned long seed);
long random(long howsmall, long howbig);
long map(long, long, long, long, long);

/* system functions */
int main( void );
void init( void );

/* sketch */
void setup( void ) ;
void loop( void ) ;

#endif // Arduino_h
