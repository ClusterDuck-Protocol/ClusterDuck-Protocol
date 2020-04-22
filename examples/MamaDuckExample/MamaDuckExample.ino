#include <ClusterDuck.h>
//#include "timer.h"

//auto timer = timer_create_default(); // create a timer with default settings

ClusterDuck duck;


//Setup LED
int ledR = 25;
int ledG = 4;
int ledB = 2;

void setupLED() {
  ledcAttachPin(ledR, 1); // assign RGB led pins to channels
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);
//  
//  // Initialize channels 
//  // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
//  // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);
}

void setColor(int red, int green, int blue)
{
  ledcWrite(1, red);
  ledcWrite(2, green);
  ledcWrite(3, blue);  
}


void setup() {
  
  // put your setup code here, to run once:
  duck.begin();
  duck.setDeviceId("R");
  duck.setupMamaDuck();
  setupLED();
  setColor(255,70,000);

  //timer.every(300000, runSensor);
}

void loop() {
  //timer.tick();
  
  // put your main code here, to run repeatedly:
  duck.runMamaDuck();
  
  
}

bool runSensor(void *) {

  return true;
}
