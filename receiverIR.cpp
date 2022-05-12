#include "receiverIR.h"
#include "pwmFan.h"

uint16_t RECV_PIN = D6; // ИК-детектор
IRrecv irrecv(RECV_PIN);
decode_results results;

ReceiverIR::ReceiverIR() {
  uint8_t vu;
}

void ReceiverIR::SETUP() {
  irrecv.enableIRIn();  // запускаем приемник
  autoMode = false;
}

void ReceiverIR::receiver() {
  if ( irrecv.decode( &results )) { // если данные пришли
    //Serial.println(results.value);
    switch (results.value) {
      case 3238126971: //*
        //case 16738455:
        autoMode = 0;
        if (pwmFan.firstFanIsActive == 1) {
          pwmFan.firstFanOFF();
        }
        else if (pwmFan.firstFanIsActive == 0) {
          pwmFan.firstFanON();
        }
        break;
      case 4039382595://#
        autoMode = 0;
        if (pwmFan.secondFanIsActive == 1) {
          pwmFan.secondFanOFF();
        }
        else if (pwmFan.secondFanIsActive == 0) {
          pwmFan.secondFanON();
        }
        break;
      case 2538093563://0
        autoMode = 0;
        pwmFan.firstFanOFF();
        pwmFan.secondFanOFF();
        pwmFan.rpmFan = 0;
        break;
      case 3810010651: //1
        autoMode = 0;
        pwmFan.rpmFan = 25.5;
        break;
      case 5316027: //2
        autoMode = 0;
        pwmFan.rpmFan = 51;
        break;
      case 4001918335://3
        autoMode = 0;
        pwmFan.rpmFan = 76.5;
        break;
      case 1386468383://4
        autoMode = 0;
        pwmFan.rpmFan = 102;
        break;
      case 3622325019://5
        autoMode = 0;
        pwmFan.rpmFan = 127.5; //50%
        break;
      case 553536955://6
        autoMode = 0;
        pwmFan.rpmFan = 153;
        break;
      case 4034314555://7
        autoMode = 0;
        pwmFan.rpmFan = 178.5;
        break;
      case 2747854299://8
        autoMode = 0;
        pwmFan.rpmFan = 204;
        break;
      case 3855596927://9
        autoMode = 0;
        pwmFan.rpmFan = 229.5;
        break;
      case 1033561079:// ^
        autoMode = 0;
        if (pwmFan.rpmFan >= 0 || pwmFan.rpmFan <= 255) {
          pwmFan.rpmFan = pwmFan.rpmFan + 5;
        }
        else {
          pwmFan.rpmFan = 255;
        }
        break;
      case 465573243:// V
        autoMode = 0;
        if (pwmFan.rpmFan >= 0 || pwmFan.rpmFan <= 255) {
          pwmFan.rpmFan = pwmFan.rpmFan - 5;
        }
        else {
          pwmFan.rpmFan = 0;
        }
        break;
      case 2351064443:// <
        autoMode = 0;
        if (pwmFan.rpmFan >= 0 || pwmFan.rpmFan <= 255) {
          pwmFan.rpmFan = pwmFan.rpmFan - 25.5;
        }
        else {
          pwmFan.rpmFan = 0;
        }
        break;
      case 71952287:// >
        autoMode = 0;
        if (pwmFan.rpmFan >= 0 || pwmFan.rpmFan <= 255) {
          pwmFan.rpmFan = pwmFan.rpmFan + 25.5;
        }
        else {
          pwmFan.rpmFan = 255;
        }
        break;
      case 1217346747:// ok
        if (autoMode == true) {
          autoMode = false;
          Serial.println(" AutoMode OFF ");
        }
        else if (autoMode == false) {
          autoMode = true;
          Serial.println(" AutoMode ON ");
        }
        break;
    }
    irrecv.resume(); // принимаем следующую команду
  }
}

ReceiverIR receiverIR = ReceiverIR();
