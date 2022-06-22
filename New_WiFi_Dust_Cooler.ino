#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

uint16_t RECV_PIN = D5; // ИК-детектор
IRrecv irrecv(RECV_PIN);
decode_results results;

const char* ssid = "AsusLyra"; //WiFI Name
const char* password = "123456qwerty"; //WiFi Password
const char* ap_ssid = "DustCooler"; //AP Name
const char* ap_password = "dustcooler"; //AP Password
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
float rpmFanSmall = 63.75;
float rpmSmallCopy;
float rpmFanCoppyBig;

//255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
//float frequencyIncrement = 25.5;//12.75;   // incremental change in PWM frequency
//int timeFrequency = 100;    // time period the PWM frequency is changing



void firstFanOFF() {
  digitalWrite(relayPin1, LOW);
  firstFanIsActive = 0;
  rpmSmallCopy = rpmFanSmall;
  rpmFanSmall = 0;
  client.publish(pub1, "0");
  Serial.println("First cooler: OFF");
}

void firstFanON() {
  digitalWrite(relayPin1, HIGH);
  firstFanIsActive = 1;
  rpmFanSmall = rpmSmallCopy;
  client.publish(pub1, "1");
  Serial.println("First cooler: ON");
}

void secondFanOFF() {
  digitalWrite(relayPin2, LOW);
  secondFanIsActive = 0;
  rpmFanCoppyBig = rpmFan;
  rpmFan = 0;
  client.publish(pub2, "0");
  Serial.println("Second cooler: OFF");
}

void secondFanON() {
  digitalWrite(relayPin2, HIGH);
  secondFanIsActive = 1;
  rpmFan = rpmFanCoppyBig;
  client.publish(pub2, "1");
  Serial.println("Second cooler: ON");
}


void setup_wifi() {
  delay(10);
  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);

    }
    Serial.println((char)payload[0]);





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
  dustDensity = 0.17 * calcVoltage;// - 0.1;
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
  if (fanPin = pwmFanOutputPin) {
    rpmFan = speed_new;
  } else if (fanPin = relayPin2) {
    rpmFanSmall = speed_new;
  }
  analogWrite(fanPin, speed_new);
  Serial.println(speed_new);
  char rpmString[8];
  dtostrf(speed_new, 1, 2, rpmString);
  Serial.print("rpmString: ");
  Serial.println(rpmString);
  client.publish(pub3, rpmString);
  return speed_new;
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
        setPwm(relayPin2, 25.5);
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
        setPwm(relayPin2, 127.5); //50%
        break;
      case 16761405://6
        setPwm(pwmFanOutputPin ,153);
        break;
      case 16769055://7
        setPwm(pwmFanOutputPin, 178.5);
        break;
      case 16754775://8
        setPwm(pwmFanOutputPin, 204);
        break;
      case 16748655://9
        setPwm(pwmFanOutputPin, 229.5);
        setPwm(relayPin2, 229.5);
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
  analogWrite(relayPin2, rpmFanSmall);

  pinMode(dustLedPower, OUTPUT);
  pinMode(dustSensorPin, INPUT);

  irrecv.enableIRIn();
  setup_wifi();

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  timerDust = millis();
  timerDustMinute = millis();
  //currentMillisWiFi = millis();
}

void loop() {
  //analogWrite(pwmFanOutputPin, rpmFan);

  if (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW); //on
    manual_control();
    reconnect();
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    manual_control();


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

}
