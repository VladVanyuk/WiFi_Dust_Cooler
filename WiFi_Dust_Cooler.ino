
// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
//#include <Hash.h>
#include <Arduino_JSON.h>
//#include "SPIFFS.h"
#include "ThingSpeak.h"

#include "dustSensor.h"
#include "receiverIR.h"
#include "pwmFan.h"

int density;
const char* PARAM_INPUT = "value";

// Replace with your network credentials
const char* ssid = "AsusLyra";
const char* password = "123456qwerty";

WiFiClient  client;

unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "OTGSHPKTRX2JQZFM";

// Stores LED state
String fan1State;
String fan2State;
String automodState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String getDensity() {
  density = dustSensor.readDustSensor();
  Serial.println(density);
  return String(density);
}
  
String getRpm() {
  float rpm = pwmFan.rpmFan;
  Serial.println(rpm);
  return String(rpm);
}

// Replaces placeholder with LED state value
String processor(const String& var){
  Serial.println(var);
  if(var == "STATEFAN1"){
    if(digitalRead(pwmFan.relayPin1)){
      fan1State = "ON";
    }
    else{
      fan1State = "OFF";
    }
    Serial.print(fan1State);
    return fan1State;
  }
  else if (var == "STATEFAN2"){
    if(digitalRead(pwmFan.relayPin2)){
      fan2State = "ON";
    }
    else{
      fan2State = "OFF";
    }
    Serial.print(fan2State);
    return fan2State;
  }
  else if (var == "DENSITY"){
    return getDensity();
  }
  else if (var == "RPM"){
    return getRpm();
  }
  else if (var == "AUTOMOD"){
    if(receiverIR.autoMode == true){
      automodState = "ON";
    }
    else{
      automodState = "OFF";
    }
    Serial.print(automodState);
    return automodState;
  }
  else if (var == "SLIDERVALUE"){
    String sliderValue = String(pwmFan.rpmFan, 2);
    return sliderValue;
    }
}
 
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dustSensor.SETUP();
  receiverIR.SETUP();
  pwmFan.SETUP();
  
  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set FAN1 to HIGH
  server.on("/fan1on", HTTP_GET, [](AsyncWebServerRequest *request){
    pwmFan.firstFanON();    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set FAN1 to LOW
  server.on("/fan1off", HTTP_GET, [](AsyncWebServerRequest *request){
    pwmFan.firstFanOFF();    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to set FAN2 to HIGH
  server.on("/fan2on", HTTP_GET, [](AsyncWebServerRequest *request){
    pwmFan.secondFanON();    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set FAN2 to LOW
  server.on("/fan2off", HTTP_GET, [](AsyncWebServerRequest *request){
    pwmFan.secondFanOFF();    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });


  server.on("/density", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getDensity().c_str());
  });
  
  server.on("/rpm", HTTP_GET, [](AsyncWebServerRequest *request){
    //analogWrite(pwmFan.pwmFanOutputPin, pwmFan.rpmFan);
    request->send_P(200, "text/plain", getRpm().c_str());
  });

   // Route to set Automod on
  server.on("/automodon", HTTP_GET, [](AsyncWebServerRequest *request){
    receiverIR.autoMode = true;   
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set Automod off
  server.on("/automodoff", HTTP_GET, [](AsyncWebServerRequest *request){
    receiverIR.autoMode = false;   
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      pwmFan.rpmFan = inputMessage.toInt();
      analogWrite(pwmFan.pwmFanOutputPin, pwmFan.rpmFan);
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
  
  WiFi.mode(WIFI_STA);    
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

 
void loop(){
  receiverIR.receiver();
  density = dustSensor.readDustSensor();
  int rpm = pwmFan.rpmFan;
  ThingSpeak.setField(1, density);
  ThingSpeak.setField(2, rpm);
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  if (receiverIR.autoMode == true){
    if (density > 500){
      pwmFan.firstFanON();
      pwmFan.secondFanON();
      int densityCopy = density;
      pwmFan.rpmFan = densityCopy/4 - 1; 
      analogWrite(pwmFan.pwmFanOutputPin, pwmFan.rpmFan);
      Serial.print("WAIT 5 min");  
      delay(300000);
    } else if (density <= 500) {
      pwmFan.firstFanOFF();
      pwmFan.secondFanOFF();
      delay(5000);
      //delay(10000);
      }
  }
   else if (receiverIR.autoMode == false){
    receiverIR.receiver();
    analogWrite(pwmFan.pwmFanOutputPin, pwmFan.rpmFan);  
    //delay(pwmFan.timeFrequency);
     }
  receiverIR.receiver();
  delay(1000);
}
