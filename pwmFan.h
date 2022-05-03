#ifndef pwmFan_h
#define pwmFan_h
#include "Arduino.h"

class PwmFan{
  public:
    PwmFan();
    void SETUP();
    const int pwmFanOutputPin = D7;   //blue wire
    const int relayPin1 = D2;
    const int relayPin2 = D3;
    float rpmFan; //= 63.75;    //255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
    float frequencyIncrement;// = 25.5;//12.75;   // incremental change in PWM frequency
    int timeFrequency;// = 100;    // time period the PWM frequency is changing
    bool firstFanIsActive;
    bool secondFanIsActive;
    void firstFanOFF();
    void firstFanON();
    void secondFanOFF();
    void secondFanON();
  private:
     
};

extern PwmFan pwmFan;

#endif
