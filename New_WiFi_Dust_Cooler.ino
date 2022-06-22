#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

uint16_t RECV_PIN = D5; // ИК-детектор
IRrecv irrecv(RECV_PIN);
decode_results results;

IPAddress apIP(192, 168, 50, 155);

const char* ssid = "AsusLyra"; //WiFI Name
const char* password = "123456qwerty"; //WiFi Password
const char* ap_ssid = "DustCoolerAP"; //AP Name
const char* ap_password = ""; //AP Password
const char* mqttServer = "192.168.50.28";
const char* mqttUserName = "MQVV"; // MQTT username
const char* mqttPwd = "123mqtt9"; // MQTT password
const char* clientID = "DustCooler"; // client id

#define sub1 "esp8266/DustCooler/switchOneCooler"
#define pub1 "esp8266/DustCooler/switchOneCooler/state"

#define sub2 "esp8266/DustCooler/switchTwoCooler"
#define pub2 "esp8266/DustCooler/switchTwoCooler/state"

#define sub3 "esp8266/DustCooler/coolerSpeed"
#define pub3 "esp8266/DustCooler/coolerSpeed"

#define pub4 "esp8266/DustCooler/dustDensity"

WiFiClient espDustCoolerClient;
PubSubClient client(espDustCoolerClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (80)
char msg[MSG_BUFFER_SIZE];
int value = 0;

const int dustSensorPin = A0;
const int dustLedPower = D6;
int voMeasured;
float calcVoltage;
float dustDensity;
unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;
unsigned long timerDust;
unsigned long timerDelay = 10000;
unsigned long timerDustMinute;
unsigned long timerDelayMinute = 60000;
unsigned long currentMillisWiFi;
unsigned long previousMillisWiFi = 0;
unsigned long intervalWiFi = 60000;
int calculateDustSix = 0;


const int pwmFanOutputPin = D7;   //blue wire
const int relayPin1 = D1;
const int relayPin2 = D2;
int firstFanIsActive ;//= LOW;
int secondFanIsActive ;//= LOW;
float rpmFan = 63.75;
float rpmFanCoppy;

String newHostname = "ESP8266DustCooler";

//255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
//float frequencyIncrement = 25.5;//12.75;   // incremental change in PWM frequency
//int timeFrequency = 100;    // time period the PWM frequency is changing



void firstFanOFF() {
  digitalWrite(relayPin1, LOW);
  firstFanIsActive = 0;
  client.publish(pub1, "0");
  Serial.println("First cooler: OFF");
}

void firstFanON() {
  digitalWrite(relayPin1, HIGH);
  firstFanIsActive = 1;
  client.publish(pub1, "1");
  Serial.println("First cooler: ON");
}

void secondFanOFF() {
  digitalWrite(relayPin2, LOW);
  secondFanIsActive = 0;
  rpmFanCoppy = rpmFan;
  rpmFan = 0;
  client.publish(pub2, "0");
  Serial.println("Second cooler: OFF");
}

void secondFanON() {
  digitalWrite(relayPin2, HIGH);
  secondFanIsActive = 1;
  rpmFan = rpmFanCoppy;
  client.publish(pub2, "1");
  Serial.println("Second cooler: ON");
}

bool StartAPMode() {
  delay(100);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println(WiFi.softAPIP());
  delay(500);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  return true;
}

bool StartSTAMode() {
  delay(10);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  //Get Current Hostname
  Serial.printf("Default hostname: %s\n", WiFi.hostname().c_str());

  //Set new hostname
  WiFi.hostname(newHostname.c_str());

  //Get Current Hostname
  Serial.printf("New hostname: %s\n", WiFi.hostname().c_str());
  WiFi.begin(ssid, password);
  delay(100);
  byte tries = 10;
  while (--tries && WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  return true;
}

void setup_wifi() {
  delay(10);
  StartSTAMode();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi up AP");
    StartAPMode();
  }
  else {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientID, mqttUserName, mqttPwd)) {
      Serial.println("MQTT connected");
      // ... and resubscribe
      client.subscribe(sub1);
      client.subscribe(sub2);
      client.subscribe(sub3);
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  if (strstr(topic, sub1))
  {
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '0') {
      firstFanOFF();

    } else if ((char)payload[0] == '1') {
      firstFanON();
    } else {
      Serial.println("Wrong command");
    }
  } else if (strstr(topic, sub2)) {
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '0') {
      secondFanOFF();
    } else if ((char)payload[0] == '1') {
      secondFanON();
    } else {
      Serial.println("Wrong command");
    }
  } else if (strstr(topic, sub3)) {
    String messageSpdStr;
    for (int i = 0; i < length; i++) {
      //  int topicValue = String((char)payload[i]).toInt();
      messageSpdStr += (char)payload[i];
      Serial.print((char)payload[i]);
    }
    Serial.println(messageSpdStr);
    int messageSpdInt = messageSpdStr.toInt();
    Serial.println(messageSpdInt);
    //setPwm(pwmFanOutputPin, messageSpdInt);
    rpmFan = messageSpdInt;
    analogWrite(pwmFanOutputPin, messageSpdInt);
  }
  else
  {
    Serial.println("unsubscribed topic");
  }
}

int getDust() {
  digitalWrite(dustLedPower, LOW);
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(dustSensorPin);// 0 - 1023

  delayMicroseconds(deltaTime);
  digitalWrite(dustLedPower, HIGH);
  delayMicroseconds(sleepTime);

  calcVoltage = voMeasured * (5 / 1024);
  dustDensity = 0.17 * calcVoltage - 0.1;
  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
  }

  Serial.println("Raw Signal Value (0-1023):");
  Serial.println(voMeasured);

  Serial.println("Voltage:");
  Serial.println(calcVoltage);

  Serial.println("Dust Density:");
  Serial.println(dustDensity);
  return voMeasured;
}

float setPwm(int fanPin, float speed_new) {
  rpmFan = speed_new;
  analogWrite(fanPin, speed_new);
  Serial.println(speed_new);
  char rpmString[8];
  dtostrf(speed_new, 1, 2, rpmString);
  Serial.print("rpmString: ");
  Serial.println(rpmString);
  client.publish(pub3, rpmString);
  return rpmFan;
}

void manual_control() {
  if ( irrecv.decode( &results )) { // если данные пришли
    Serial.println(results.value);
    switch (results.value) {
      case 16738455: //*
        if (firstFanIsActive == 1) {
          firstFanOFF();
        }
        else if (firstFanIsActive == 0) {
          firstFanON();
        }
        break;
      case 16756815://#
        if (secondFanIsActive == 1) {
          secondFanOFF();
        }
        else if (secondFanIsActive == 0) {
          secondFanON();
        }
        break;
      case 16750695://0
        firstFanOFF();
        secondFanOFF();
        setPwm(pwmFanOutputPin, 0);
        break;
      case 16753245: //1
        setPwm(pwmFanOutputPin, 25.5);
        break;
      case 16736925: //2
        setPwm(pwmFanOutputPin, 51);
        break;
      case 16769565://3
        setPwm(pwmFanOutputPin, 76.5);
        break;
      case 16720605://4
        setPwm(pwmFanOutputPin, 102);
        break;
      case 16712445://5
        setPwm(pwmFanOutputPin, 127.5); //50%
        break;
      case 16761405://6
        setPwm(pwmFanOutputPin , 153);
        break;
      case 16769055://7
        setPwm(pwmFanOutputPin, 178.5);
        break;
      case 16754775://8
        setPwm(pwmFanOutputPin, 204);
        break;
      case 16748655://9
        setPwm(pwmFanOutputPin, 229.5);
        break;
      case 16718055:// ^
        if (rpmFan >= 0 || rpmFan <= 255) {
          setPwm(pwmFanOutputPin, rpmFan + 5);

        }
        else {
          setPwm(pwmFanOutputPin, 255);
        }
        break;
      case 16730805:// V
        if (rpmFan >= 0 || rpmFan <= 255) {
          setPwm(pwmFanOutputPin, rpmFan - 5);
        }
        else {
          setPwm(pwmFanOutputPin, 0);
        }
        break;
      case 16716015:// <
        if (rpmFan >= 0 || rpmFan <= 255) {
          setPwm(pwmFanOutputPin, rpmFan - 25);
        }
        else {
          setPwm(pwmFanOutputPin, 0);
        }
        break;
      case 16734885:// >
        if (rpmFan >= 0 || rpmFan <= 255) {
          setPwm(pwmFanOutputPin, rpmFan + 25);
        }
        else {
          setPwm(pwmFanOutputPin, 255);
        }
        break;
      case 16726215:// ok
        if (firstFanIsActive == 1) {
          firstFanOFF();
        }
        else if (firstFanIsActive == 0) {
          firstFanON();
        }

        if (secondFanIsActive == 1) {
          secondFanOFF();
        }
        else if (secondFanIsActive == 0) {
          secondFanON();
        }

        break;
    }
    irrecv.resume(); // принимаем следующую команду
  }
  //analogWrite(pwmFanOutputPin, rpmFan);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, LOW); //off
  digitalWrite(relayPin2, LOW); // off
  firstFanIsActive = 0;//= LOW;
  secondFanIsActive = 0;//= LOW;
  pinMode(pwmFanOutputPin, OUTPUT);
  analogWrite(pwmFanOutputPin, rpmFan);


  pinMode(dustLedPower, OUTPUT);

  irrecv.enableIRIn();
  setup_wifi();

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  timerDust = millis();
  timerDustMinute = millis();
  //currentMillisWiFi = millis();
}

void loop() {
  manual_control();

  if (WiFi.status() == WL_CONNECTED)
  {
    if (WiFi.mode(WIFI_STA)) {
      if (!client.connected()) {
        digitalWrite(LED_BUILTIN, LOW); //on
        reconnect();
      } else {
        digitalWrite(LED_BUILTIN, HIGH);
        if (millis() - timerDust > timerDelay) { //10 sec
          calculateDustSix += getDust();
          Serial.println("speed cooler: ");
          Serial.println(rpmFan);
          timerDust = millis();
        }

        if (millis() - timerDustMinute > timerDelayMinute) { //60 sec
          int everageDustVal = calculateDustSix / 6;
          char everageDustStr[8];
          dtostrf(everageDustVal, 1, 2, everageDustStr);
          Serial.print("Dust density: ");
          Serial.println(everageDustVal);
          client.publish(pub4, everageDustStr , true); // retained
          calculateDustSix = 0;
          timerDustMinute = millis();
        }
        client.loop();
      }
    } else if (WiFi.mode(WIFI_AP)) {
      ArduinoOTA.handle();
      delay(500);
    } else {
      setup_wifi();
      delay(500);
    }
  } else {
    Serial.println("Starting AP");
    StartAPMode();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }

    delay(1000);
  }
}
