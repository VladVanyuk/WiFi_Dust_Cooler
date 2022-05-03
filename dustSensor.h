#ifndef dustSensor_h
#define dustSensor_h

class DustSensor{
  public:
    DustSensor();
    void SETUP();
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
