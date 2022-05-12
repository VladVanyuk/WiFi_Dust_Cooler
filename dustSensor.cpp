#include "Arduino.h"
#include <EEPROM.h>
#include "dustSensor.h"

const int dustSensorPin = A0;
const int dustLedPower = D5;
unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;

DustSensor::DustSensor() {
  uint8_t vu;
}

void DustSensor::SETUP() {
  pinMode(dustLedPower, OUTPUT);
  pinMode(dustSensorPin, INPUT);
  digitalWrite(dustLedPower, HIGH); //off
  voMeasured = 0;
  calcVoltage = 0;
  dustDensity = 0;
}


int DustSensor::readDustSensor() {
  digitalWrite (dustLedPower, LOW); //  power on the LED
  delayMicroseconds (samplingTime);
  voMeasured =  analogRead (dustSensorPin);  //  read the dust value
  delayMicroseconds (deltaTime);
  digitalWrite (dustLedPower, HIGH); //  turn the LED off
  delayMicroseconds (sleepTime);

  calcVoltage = voMeasured * (5.0 / 1024);
  dustDensity = 0.17 * calcVoltage - 0.1;

  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
  }
  return  voMeasured;
}

DustSensor dustSensor = DustSensor();
