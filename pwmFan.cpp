#include <EEPROM.h>
#include "pwmFan.h"
#include "dustSensor.h"

PwmFan::PwmFan(){
  uint8_t vu;
}

void PwmFan::SETUP(){
  pinMode(pwmFanOutputPin, OUTPUT);  
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, LOW); //off
  digitalWrite(relayPin2, LOW); // off
  firstFanIsActive = 0;
  secondFanIsActive = 0;
  rpmFan = 63.75;
  frequencyIncrement = 25.5;
  timeFrequency = 500; //100
}


void PwmFan::firstFanOFF(){
  digitalWrite(relayPin1, LOW);
  firstFanIsActive = 0;
  }
  
void PwmFan::firstFanON(){
  digitalWrite(relayPin1, HIGH);
  firstFanIsActive = 1;
  }

void PwmFan::secondFanOFF(){
  digitalWrite(relayPin2, LOW);
  secondFanIsActive = 0;
  }

void PwmFan::secondFanON(){
  digitalWrite(relayPin2, HIGH);
  secondFanIsActive = 1;
  }

PwmFan pwmFan = PwmFan();
