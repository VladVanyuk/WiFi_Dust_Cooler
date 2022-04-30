#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "index.h"  //Web page

uint16_t RECV_PIN = D6; // ИК-детектор
IRrecv irrecv(RECV_PIN);
decode_results results;

const int pwmFanOutputPin = D7;   //blue wire
const int relayPin1 = D2;
const int relayPin2 = D3;
float rpmFan = 63.75;    //255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
float frequencyIncrement = 25.5;//12.75;   // incremental change in PWM frequency
int timeFrequency = 100;    // time period the PWM frequency is changing
bool firstFanIsActive;
bool secondFanIsActive;

void firstFanOFF();
void firstFanON();
void secondFanOFF();
void secondFanON();

//SSID and Password of your WiFi router
const char* ssid = "AsusLyra";
const char* password = "123456qwerty";
AsyncWebServer server(80);

const char* PARAM_INPUT = "value";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  if (var == "SLIDERVALUE"){
    String sliderValue = String(rpmFan, 2);
    return sliderValue;
  }
  return String();
}

void setup()
{
  Serial.begin(115200);
  pinMode(pwmFanOutputPin, OUTPUT);  
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, LOW); //off
  digitalWrite(relayPin2, LOW); // off
  firstFanIsActive = false;
  secondFanIsActive = false;
  irrecv.enableIRIn();  // запускаем приемник

  WiFi.begin(ssid, password);     //Connect to your WiFi router
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  //If connection successful show IP address in serial monitor
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });  
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      rpmFan = inputMessage.toInt();                                              
      analogWrite(pwmFanOutputPin, rpmFan);
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  server.begin();
}

void loop()
{
  analogWrite(pwmFanOutputPin, rpmFan);  
  delay(timeFrequency);
  receiver();
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
  digitalWrite(relayPin1, LOW);
  secondFanIsActive = false;
  }

void secondFanON(){
  digitalWrite(relayPin1, HIGH);
  secondFanIsActive = true;
  }

void receiver(){
  if ( irrecv.decode( &results )) { // если данные пришли
    switch ( results.value) {
    case 3238126971://*
        firstFanON();
        break;
    case 4039382595://#
        firstFanOFF();
        break;
    case 2538093563://0
        rpmFan = 0;
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
        if(firstFanIsActive == true){ firstFanOFF(); }
        else if(firstFanIsActive == false){ firstFanON(); }
        break;                                           
    }
  irrecv.resume(); // принимаем следующую команду                  
  }
 }
