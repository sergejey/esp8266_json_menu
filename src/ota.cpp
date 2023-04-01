#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

void startOTA() {
  ArduinoOTA.begin();
}

void handleOTA() {
  ArduinoOTA.handle();
}