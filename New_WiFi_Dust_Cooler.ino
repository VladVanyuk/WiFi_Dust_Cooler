#include "functions_control.h"
#include "settings.h"


void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(pwmFanOutputPin, OUTPUT);
  digitalWrite(relayPin1, LOW); //off
  digitalWrite(relayPin2, LOW); // off
  firstFanIsActive = 0;//= LOW;
  secondFanIsActive = 0;//= LOW;
  rpmFanOne = 0;
  rpmFanTwo = 0;
  analogWrite(pwmFanOutputPin, rpmFanOne);
  analogWrite(relayPin2, rpmFanTwo);
  irrecv.enableIRIn();
  setup_wifi();

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

void loop() {
  manual_control();


  if (WiFi.status() == WL_CONNECTED && WiFi.mode(WIFI_STA)) {
    if (!client.connected()) {
      digitalWrite(LED_BUILTIN, LOW); //on
      if (triedReconnect == false) {
        reconnect();
        triedReconnect = true;
      }
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      if (millis() % 500 == 0) client.loop();
    }
  } else if (WiFi.status() == WL_CONNECTED && WiFi.mode(WIFI_AP)) {
    if (millis() % 1000 == 0) {
      ArduinoOTA.handle();
    }
  } else if (WiFi.status() != WL_CONNECTED && triedAPmode == false) {
    StartAPMode();
    triedAPmode = true;
  }
}
