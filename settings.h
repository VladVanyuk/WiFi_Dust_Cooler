#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>


IPAddress apIP(192, 168, 50, 155);

#define ssid "AsusLyra" //WiFI Name
#define password "123456qwerty" //WiFi Password
#define ap_ssid "DustCoolerAP" //AP Name
#define  ap_password "" //AP Password
#define  mqttServer "192.168.50.28"
#define  mqttUserName "MQVV" // MQTT username
#define  mqttPwd "123mqtt9" // MQTT password
#define clientID "DustCooler" // client id

/*
const char* ssid = "AsusLyra"; //WiFI Name
const char*  password = "123456qwerty"; //WiFi Password
const char* ap_ssid = "DustCoolerAP"; //AP Name
const char*  ap_password = ""; //AP Password
const char*  mqttServer = "192.168.50.28";
const char*  mqttUserName = "MQVV"; // MQTT username
const char*  mqttPwd = "123mqtt9"; // MQTT password
const char*  clientID = "DustCooler"; // client id
*/

WiFiClient espDustCoolerClient;
PubSubClient client(espDustCoolerClient);
#define sub1 "esp8266/DustCooler/switchOneCooler"
#define pub1 "esp8266/DustCooler/switchOneCooler/state"

#define sub2 "esp8266/DustCooler/switchTwoCooler"
#define pub2 "esp8266/DustCooler/switchTwoCooler/state"

#define sub3 "esp8266/DustCooler/coolerSpeedFirst"
#define pub3 "esp8266/DustCooler/coolerSpeedFirst"

#define sub4 "esp8266/DustCooler/coolerSpeedSecond"
#define pub4 "esp8266/DustCooler/coolerSpeedSecond"
