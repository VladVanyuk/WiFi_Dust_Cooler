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

#define sub3 "esp8266/DustCooler/coolerSpeedFirst"
#define pub3 "esp8266/DustCooler/coolerSpeedFirst"

#define sub4 "esp8266/DustCooler/coolerSpeedSecond"
#define pub4 "esp8266/DustCooler/coolerSpeedSecond"

WiFiClient espDustCoolerClient;
PubSubClient client(espDustCoolerClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (80)
char msg[MSG_BUFFER_SIZE];
int value = 0;

const int relayPin1 = D1;
const int relayPin2 = D2;
const int pwmFanOutputPin = D6;   //blue wire
int firstFanIsActive ;//= LOW;
int secondFanIsActive ;//= LOW;
float rpmFanOne = 63.75;
float rpmFanTwo = 25;
float rpmFanOneCoppy;
float rpmFanTwoCoppy;

String newHostname = "ESP8266DustCooler";

//255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
//float frequencyIncrement = 25.5;//12.75;   // incremental change in PWM frequency
//int timeFrequency = 100;    // time period the PWM frequency is changing

void firstFanOFF() {
  digitalWrite(relayPin1, LOW);
  firstFanIsActive = 0;
  Serial.println("First cooler: OFF");
}

void firstFanOFFPublish() {
  firstFanOFF();
  client.publish(pub1, "0");
  //client.publish(pub3, "0");
}

void firstFanON() {
  digitalWrite(relayPin1, HIGH);
  firstFanIsActive = 1;
  Serial.println("First cooler: ON");
}

void firstFanONPublish() {
  firstFanON();
  client.publish(pub1, "1");
  //client.publish(pub3, "1");
}

void secondFanOFF() {
  rpmFanTwoCoppy = rpmFanTwo;
  rpmFanTwo = 0;
  analogWrite(relayPin2, LOW);
  secondFanIsActive = 0;
  Serial.println("Second cooler: OFF");
}

void secondFanOFFPublish() {
  secondFanOFF();
  client.publish(pub2, "0");
  //client.publish(pub4, "0");
}

void secondFanON() {
  if (rpmFanTwo == 0) {
    rpmFanTwo = rpmFanTwoCoppy;
  }
  analogWrite(relayPin2, rpmFanTwo);
  secondFanIsActive = 1;
  Serial.println("Second cooler: ON");
}


void secondFanONPublish() {
  secondFanON();
  client.publish(pub2, "1");
  //client.publish(pub4, "1");
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
  byte tries = 30;
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
      client.subscribe(sub4);
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
      messageSpdStr += (char)payload[i];
      Serial.print((char)payload[i]);
    }
    Serial.println(messageSpdStr);
    int messageSpdInt = messageSpdStr.toInt();
    Serial.println(messageSpdInt);
    rpmFanOne = messageSpdInt;
    analogWrite(pwmFanOutputPin, messageSpdInt);
  } else if (strstr(topic, sub4)) {
    String messageSpd2Str;
    for (int i = 0; i < length; i++) {
      messageSpd2Str += (char)payload[i];
      Serial.print((char)payload[i]);
    }
    Serial.println(messageSpd2Str);
    int messageSpd2Int = messageSpd2Str.toInt();
    Serial.println(messageSpd2Int);
    rpmFanTwo = messageSpd2Int;
    analogWrite(relayPin2, messageSpd2Int);
  } else {
    Serial.println("unsubscribed topic");
  }
}

float setPwm(int fanPin, float speed_new) {
  if (fanPin == pwmFanOutputPin) {
    rpmFanOne = speed_new;
  } else if (fanPin == relayPin2) {
    rpmFanTwo = speed_new;
  }
  analogWrite(fanPin, speed_new);
  Serial.println(speed_new);
  char rpmString[8];
  dtostrf(speed_new, 1, 2, rpmString);
  Serial.print("rpmString: ");
  Serial.println(rpmString);
  if (fanPin == pwmFanOutputPin) {
    client.publish(pub3, rpmString);
  } else if (fanPin == relayPin2) {
    client.publish(pub4, rpmString);
  }
  return speed_new;
}

void manual_control() {
  if ( irrecv.decode( &results )) { // если данные пришли
    Serial.println(results.value);
    switch (results.value) {
      case 50163855: //*
        if (firstFanIsActive == 1) {
          firstFanOFFPublish();
        }
        else if (firstFanIsActive == 0) {
          firstFanONPublish();
        }
        break;
      case 50147535://#
        if (secondFanIsActive == 1) {
          secondFanOFFPublish();
        }
        else if (secondFanIsActive == 0) {
          secondFanONPublish();
        }
        break;
      case 50135295://0
        firstFanOFFPublish();
        secondFanOFFPublish();
        break;
      case 50167935: //1
        setPwm(pwmFanOutputPin, 25.5);
        break;
      case 50151615: //2
        setPwm(pwmFanOutputPin, 51);
        setPwm(relayPin2, 51);
        break;
      case 50184255://3
        setPwm(pwmFanOutputPin, 76.5);
        setPwm(relayPin2, 76.5);
        break;
      case 50143455://4
        setPwm(pwmFanOutputPin, 102);
        setPwm(relayPin2, 102);
        break;
      case 50176095://5
        setPwm(pwmFanOutputPin, 127.5); //50%
        setPwm(relayPin2, 127.5);
        break;
      case 50159775://6
        setPwm(pwmFanOutputPin , 153);
        setPwm(relayPin2, 153);
        break;
      case 50192415://7
        setPwm(pwmFanOutputPin, 178.5);
        setPwm(relayPin2, 178.5);
        break;
      case 50139375://8
        setPwm(pwmFanOutputPin, 204);
        setPwm(relayPin2, 204);
        break;
      case 50172015://9
        setPwm(pwmFanOutputPin, 229.5);
        setPwm(relayPin2, 229.5);
        break;
      case 50157735:// ^
        if (rpmFanOne >= 0 || rpmFanOne <= 255) {
          setPwm(pwmFanOutputPin, rpmFanOne + 5);
          setPwm(relayPin2, rpmFanTwo + 5);
        }
        else {
          setPwm(pwmFanOutputPin, 255);
          setPwm(relayPin2, 255);
        }
        break;
      case 50165895:// V
        if (rpmFanOne >= 0 || rpmFanOne <= 255) {
          setPwm(pwmFanOutputPin, rpmFanOne - 5);
          setPwm(relayPin2, rpmFanTwo - 5);
        }
        else {
          setPwm(pwmFanOutputPin, 0);
          setPwm(relayPin2, 0);
        }
        break;
      case 16716015:// <
        if (rpmFanOne >= 0 || rpmFanOne <= 255) {
          setPwm(pwmFanOutputPin, rpmFanOne - 25);
        }
        else {
          setPwm(pwmFanOutputPin, 0);
        }
        break;
      case 16734885:// >
        if (rpmFanOne >= 0 || rpmFanOne <= 255) {
          setPwm(pwmFanOutputPin, rpmFanOne + 25);
        }
        else {
          setPwm(pwmFanOutputPin, 255);
        }
        break;
      case 50153655:// ok
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
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(pwmFanOutputPin, OUTPUT);
  digitalWrite(relayPin1, LOW); //off
  analogWrite(pwmFanOutputPin, rpmFanOne);
  analogWrite(relayPin2, rpmFanTwo);
  //digitalWrite(relayPin2, LOW); // off
  firstFanIsActive = 0;//= LOW;
  secondFanIsActive = 1;//= LOW;

  irrecv.enableIRIn();
  setup_wifi();

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
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
        client.loop();
        delay(500);
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

    delay(500);
  }
}
