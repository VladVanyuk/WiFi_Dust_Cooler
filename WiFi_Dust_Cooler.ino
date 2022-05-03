#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

uint16_t RECV_PIN = D6; // ИК-детектор
IRrecv irrecv(RECV_PIN);
decode_results results;

const int dustSensorPin = A0;
const int dustLedPower = D5;
const int pwmFanOutputPin = D7;   //blue wire
const int relayPin1 = D2;
const int relayPin2 = D3;
float rpmFan = 63.75;    //255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
float frequencyIncrement = 25.5;//12.75;   // incremental change in PWM frequency
int timeFrequency = 100;    // time period the PWM frequency is changing
bool firstFanIsActive;
bool secondFanIsActive;
int voMeasured;
float calcVoltage;
float dustDensity;
bool autoMode;

void firstFanOFF();
void firstFanON();
void secondFanOFF();
void secondFanON();

void setup()
{
  Serial.begin(115200);
  //dustSensor.SETUP();
  pinMode(pwmFanOutputPin, OUTPUT);  
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, LOW); //off
  digitalWrite(relayPin2, LOW); // off
  firstFanIsActive = false;
  secondFanIsActive = false;
  voMeasured = 0;
  calcVoltage = 0;
  dustDensity = 0;
  autoMode = false;
  irrecv.enableIRIn();  // запускаем приемник

  pinMode(dustLedPower, OUTPUT);
  pinMode(dustSensorPin, INPUT);
  digitalWrite(dustLedPower,HIGH); //off
}

void loop()
{
  receiver(); 
  if (autoMode == true){
    readDustSensor();
    Serial.print("dust ");
    Serial.println(voMeasured);
    if (voMeasured > 500){
      firstFanON();
      secondFanON();
      rpmFan = voMeasured/4 - 1; 
      analogWrite(pwmFanOutputPin, rpmFan);
      Serial.print("WAIT 5 min");  
      delay(300000);
    } else if (voMeasured <= 500) {
      firstFanOFF();
      secondFanOFF();
      delay(10000);
      }
  }
   
  while (autoMode == false){
    receiver();
    analogWrite(pwmFanOutputPin, rpmFan);  
    delay(timeFrequency);
    receiver();
    }
  receiver();
  delay(3000);
}

void firstFanOFF(){
  digitalWrite(relayPin1, LOW);
  firstFanIsActive = false;
  }

void firstFanON(){
  digitalWrite(relayPin1, HIGH);
  firstFanIsActive = true;
  }

void secondFanOFF(){
  digitalWrite(relayPin2, LOW);
  secondFanIsActive = false;
  }

void secondFanON(){
  digitalWrite(relayPin2, HIGH);
  secondFanIsActive = true;
  }

void receiver(){
  if ( irrecv.decode( &results )) { // если данные пришли
    Serial.println(results.value);
    switch ( results.value) {
    case 3238126971://*
        if(firstFanIsActive == true){ firstFanOFF(); }
        else if(firstFanIsActive == false){ firstFanON(); }
        break;
    case 4039382595://#
        if(secondFanIsActive == true){ secondFanOFF(); }
        else if(secondFanIsActive == false){  secondFanON(); }
        break;
    case 2538093563://0
        firstFanOFF();
        secondFanOFF();
        break;
    case 3810010651: //1
        rpmFan = 25.5;
        break;
    case 5316027: //2
        rpmFan = 51;
        break;
    case 4001918335://3
        rpmFan = 76.5; 
        break;
    case 1386468383://4
        rpmFan = 102;
        break;
    case 3622325019://5
        rpmFan = 127.5; //50%
        break;
    case 553536955://6
        rpmFan = 153;
        break;
    case 4034314555://7
        rpmFan = 178.5;
        break;   
    case 2747854299://8
        rpmFan = 204;
        break;
    case 3855596927://9
        rpmFan = 229.5; 
        break;
    case 1033561079:// ^
        if (rpmFan >=0 || rpmFan <=255){ rpmFan = rpmFan + 5; }
        else {rpmFan = 255;}
        break;
    case 465573243:// V
        if (rpmFan >=0 || rpmFan <=255){ rpmFan = rpmFan - 5; }
        else {rpmFan = 0;}
        break;
    case 2351064443:// <
        if (rpmFan >=0 || rpmFan <=255){ rpmFan = rpmFan - 25.5; }
        else {rpmFan = 0;}
        break;
    case 71952287:// >
        if (rpmFan >=0 || rpmFan <=255){ rpmFan = rpmFan + 25.5; }
        else {rpmFan = 255;}
        break;   
    case 1217346747:// ok
        autoMode = !autoMode;
        Serial.println("autoMode " + autoMode);
        break;                                           
    }
  irrecv.resume(); // принимаем следующую команду                  
  }
 }

int readDustSensor() {
  digitalWrite (dustLedPower,LOW);  //  power on the LED 
  delayMicroseconds (280); 
  voMeasured =  analogRead (dustSensorPin);  //  read the dust value 
  delayMicroseconds (40); 
  digitalWrite (dustLedPower,HIGH);  //  turn the LED off 
  delayMicroseconds (9680);

  calcVoltage = voMeasured*(5.0/1024);
  dustDensity = 0.17*calcVoltage-0.1;

  if ( dustDensity < 0)
  {
   dustDensity = 0.00; 
  }

  return  voMeasured; 
 }