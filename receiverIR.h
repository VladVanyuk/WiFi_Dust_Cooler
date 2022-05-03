#ifndef receiverIR_h
#define receiverIR_h
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

class ReceiverIR{
  public:
    ReceiverIR();
    void SETUP();
    bool autoMode;
    void receiver();
  private:
     
};

extern ReceiverIR receiverIR;

#endif
