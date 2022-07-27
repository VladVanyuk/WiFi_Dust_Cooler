#include "settings.h"
bool triedAPmode = false;
bool triedReconnect = false;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (80)
char msg[MSG_BUFFER_SIZE];
int value = 0;


uint16_t RECV_PIN = D5; // ИК-детектор
IRrecv irrecv(RECV_PIN);
decode_results results;
const int relayPin1 = D1;
const int relayPin2 = D6;
const int pwmFanOutputPin = D7;   //blue wire
int firstFanIsActive = 0;//= LOW;
int secondFanIsActive = 0;//= LOW;
float rpmFanOne = 0;
float rpmFanTwo = 0;

unsigned long time_reconnect = 0;
unsigned long time_ir = 0;

String newHostname = "ESP8266DustCooler";
//255 = 100%  191.25 = 75%  127.5 = 50%  63.75 = 25%  0 = 0%
//float frequencyIncrement = 25.5;//12.75;   // incremental change in PWM frequency
//int timeFrequency = 100;    // time period the PWM frequency is changing

void firstFanOFF() {
  digitalWrite(relayPin1, LOW);
  rpmFanOne = 0;
  analogWrite(pwmFanOutputPin, rpmFanOne);
  firstFanIsActive = 0;
  Serial.println("First cooler: OFF");
}

void firstFanOFFPublish() {
  if (firstFanIsActive != 0) {
    firstFanOFF();
    client.publish(pub1, "0");
    client.publish(pub3, "0");
  }
}

void firstFanON() {
  digitalWrite(relayPin1, HIGH);
  //rpmFanOne = 25;
  if (rpmFanOne != 0) {
    analogWrite(pwmFanOutputPin, rpmFanOne);
  } else {
    rpmFanOne = 25;
    analogWrite(pwmFanOutputPin, rpmFanOne);
  }
  firstFanIsActive = 1;
  Serial.println("First cooler: ON");
}

void firstFanONPublish() {
  if (firstFanIsActive != 1) {
    firstFanON();
    client.publish(pub1, "1");
  }
}

void secondFanOFF() {
  rpmFanTwo = 0;
  analogWrite(relayPin2, rpmFanTwo);
  secondFanIsActive = 0;
  Serial.println("Second cooler: OFF");
}

void secondFanOFFPublish() {
  if (secondFanIsActive != 0) {
    secondFanOFF();
    client.publish(pub2, "0");
    client.publish(pub4, "0");
  }
}

void secondFanON() {
  digitalWrite(relayPin2, HIGH);
  //rpmFanTwo = 255;
  //analogWrite(relayPin2, rpmFanTwo);
  secondFanIsActive = 1;
  Serial.println("Second cooler: ON");
}


void secondFanONPublish() {
  if (secondFanIsActive != 1) {
    secondFanON();
    client.publish(pub2, "1");
    //client.publish(pub4, "1");
  }
}

float setPwm(int fanPin, float speed_new) {
  if (fanPin == pwmFanOutputPin) {
    rpmFanOne = speed_new;
    analogWrite(fanPin, rpmFanOne);
  } else if (fanPin == relayPin2) {
    rpmFanTwo = speed_new;
    analogWrite(fanPin, rpmFanTwo);
  }
  float speed_to_publish = map(speed_new, 0, 255, 0, 100);
  char rpmString[8];
  dtostrf(speed_to_publish, 1, 2, rpmString);
  Serial.print("rpmString: ");
  Serial.println(rpmString);
  if (speed_new != 0) {
    if (fanPin == pwmFanOutputPin) {
      firstFanONPublish();
      client.publish(pub3, rpmString);
    } else if (fanPin == relayPin2) {
      secondFanONPublish();
      client.publish(pub4, rpmString);
    }
  } else {
    if (fanPin == pwmFanOutputPin) {
      firstFanOFFPublish();
    } else if (fanPin == relayPin2) {
      secondFanOFFPublish();
    }
  }

  Serial.println(speed_new);
  return speed_new;
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
  byte tries = 15;
  while (tries > 0 && WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    tries -= 1;
    if (tries == 0) {
      return false;
    }
  }
  return true;
}

void setup_wifi() {
  digitalWrite(LED_BUILTIN, LOW);
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
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void reconnect() {
  byte tries2 = 15;
  while (tries2 > 0 && !client.connected()) {
    time_reconnect = millis();
    if (client.connect(clientID, mqttUserName, mqttPwd)) {
      Serial.println("MQTT connected");
      // ... and resubscribe
      client.subscribe(sub1);
      client.subscribe(sub2);
      client.subscribe(sub3);
      client.subscribe(sub4);
    } else {
      if (millis() - time_reconnect > 5000) {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        tries2 -= 1;
        // Wait 5 seconds before retrying
        if (tries2 == 0) {
          Serial.println(" stoped reconnection ...");
          break;
        }
      }
    }
  }
}



void callback(char* topic, byte * payload, unsigned int length) {
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
      // firstFanOFF();
      if (rpmFanOne != 0) {
        setPwm(pwmFanOutputPin, 0);
      } else {
        firstFanOFF();
      }
    } else if ((char)payload[0] == '1') {
      if (rpmFanOne == 0) {
        setPwm(pwmFanOutputPin, 25);
      } else {
        firstFanON();
      }
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
      // secondFanOFF();
      if (rpmFanTwo != 0) {
        setPwm(relayPin2, 0);
      } else {
        secondFanOFF();
      }
    } else if ((char)payload[0] == '1') {
      if (rpmFanTwo == 0) {
        setPwm(relayPin2, 255);
      } else {
        secondFanON();
      }
    } else {
      Serial.println("Wrong command");
    }
  } else if (strstr(topic, sub3)) {
    String messageSpdStr;
    for (int i = 0; i < length; i++) {
      messageSpdStr += (char)payload[i];
      Serial.print((char)payload[i]);
    }
    int messageSpdInt = messageSpdStr.toInt();
    Serial.println(messageSpdInt);
    if (messageSpdInt == 0) {
      firstFanOFFPublish();
    } else {
      rpmFanOne = map(messageSpdInt, 0, 100, 0, 255);
      firstFanONPublish();
      analogWrite(pwmFanOutputPin, rpmFanOne);
    }
  } else if (strstr(topic, sub4)) {
    String messageSpd2Str;
    for (int i = 0; i < length; i++) {
      messageSpd2Str += (char)payload[i];
      Serial.print((char)payload[i]);
    }
    int messageSpd2Int = messageSpd2Str.toInt();
    Serial.println(messageSpd2Int);
    if (messageSpd2Int == 0) {
      secondFanOFFPublish();
    } else {
      rpmFanTwo = map(messageSpd2Int, 0, 100, 0, 255);
      secondFanONPublish();
      analogWrite(relayPin2, rpmFanTwo);
    }
  } else {
    Serial.println("unsubscribed topic");
  }
}


void manual_control() {
  if ( irrecv.decode( &results ) && millis() % 500 == 0) { // если данные пришли
    Serial.println(results.value);
    bool ir_control = false;
    //  time_ir = millis();
    switch (results.value) {
      case 537004346: //ch1
        if (firstFanIsActive == 1) {
          firstFanOFFPublish();
        }
        else if (firstFanIsActive == 0) {
          if (rpmFanOne == 0) {
            setPwm(pwmFanOutputPin, 25);
          } else {
            firstFanONPublish();
          }
        }
        //   ir_control = true;
        // time_ir = millis();
        break;
      case 1841421431://ch2
        if (secondFanIsActive == 1) {
          secondFanOFFPublish();
        }
        else if (secondFanIsActive == 0) {
          if (rpmFanTwo == 0) {
            setPwm(relayPin2, 255);
          } else {
            secondFanONPublish();
          }
        }
        // ir_control = true;
        //  time_ir = millis();
        break;
      case 2121296640:   //scan
        if (WiFi.status() != WL_CONNECTED) {
          triedAPmode = false;
          setup_wifi();
        } else if (WiFi.status() == WL_CONNECTED && !client.connected()) {
          triedReconnect = false;
          reconnect();
        } else {
          ESP.restart();
        }
        //  ir_control = true;
        // time_ir = millis();
        break;
    }
    irrecv.resume();
  }
}
