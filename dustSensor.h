#ifndef dustSensor_h
#define dustSensor_h

class DustSensor{
  public:
    DustSensor();
    void SETUP();
   // const int dustSensorPin = A0;
  //  const int dustLedPower = D5;
    int readDustSensor();
    int voMeasured;
    float calcVoltage;
    float dustDensity;
    
  private:
    unsigned int samplingTime;
    unsigned int deltaTime;
    unsigned int sleepTime;    
};

extern DustSensor dustSensor;

#endif
