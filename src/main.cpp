#include <Arduino.h>

#include <config.h>

#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

#ifdef ENABLE_OTA
#include <ota.h>
#endif

#include <menu_com.h>

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial)
    ;

  WiFiManager ESP_wifiManager;
  ESP_wifiManager.setConfigPortalTimeout(60);
  if (ESP_wifiManager.autoConnect("ESP_Menu"))
  {
#ifdef ENABLE_OTA
    startOTA();
#endif
    startMenu();
  }

  Serial.println("setup done.");
  Serial.flush();
}

void loop()
{
  handleMenu();

#ifdef ENABLE_OTA
  handleOTA();
#endif

  delay(100);
}
