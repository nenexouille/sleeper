#include <Arduino.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>


#define PIN_LED_SLEEP 25
#define PIN_LED_WAKE 27
#define PIN_BUTTON_SETUP 32

#define WIFI_CONFIG_SSID "SleeperConfig"
#define NTP_SERVER "time.google.com"


/////////////////////////////////////////:

struct ntp {
    NTPClient * client;
} ;