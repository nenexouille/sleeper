#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

const char* ssid     = "****";
const char* password = "****";

const int ledSleep = 25;
const int ledWake  = 27;
const int setupButton = 32;

bool setupMode = false;
DynamicJsonDocument jsonPlanning(8176);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "mafreebox.free.fr");

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void connectWiFi()
{
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void disconnectWiFi()
{
    WiFi.mode(WIFI_OFF);
}

void initTime()
{
    timeClient.begin();
    timeClient.setTimeOffset(3600);

    while(!timeClient.update()) {
        timeClient.forceUpdate();
    }
}

bool isItTimeForWakeUp()
{
  return((bool) jsonPlanning[timeClient.getDay()][timeClient.getHours()][timeClient.getMinutes()<30?0:1]);
}

void loadPlanningFile()
{
  Serial.println("Opening file");
  File planningFile = SPIFFS.open("/planning.json", FILE_READ );
  
  Serial.println("Deserializing...");
  DeserializationError err = deserializeJson(jsonPlanning, planningFile);

  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
  }
  planningFile.close();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;

  pinMode(setupButton,INPUT_PULLUP);

  if (!SPIFFS.begin()) {
    Serial.println("ERROR while initializing SPIFFS");
    return;
  }

  loadPlanningFile();
  
  connectWiFi();

  if (!digitalRead(setupButton)) {
    Serial.println("Entering SETUP MODE ...");
    setupMode = true;
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/sleeper.html", "text/html");
    });
    server.on("/planning.json", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/planning.json", "application/json");
    });
    server.on("/sleeper.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/sleeper.js", "application/javascript");
    });
    server.on("/sleeper.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/sleeper.css", "text/css");
    });
    server.on("/updatePlanning", HTTP_POST, [](AsyncWebServerRequest *request){
      String message;
      Serial.println("Nb args: " + request->args());
      if (request->params()) {
          message = request->getParam(0)->value();
          //Serial.println(message);
          
          File planningFile = SPIFFS.open("/planning.json", FILE_WRITE );
          if (planningFile.print(message)) {
            Serial.println("File was written");
          } else {
            Serial.println("File write failed");
          }
          planningFile.close();
          
      } else {
          message = "No message sent";
      }
      request->send(200, "text/plain", "");
    });

    server.onNotFound(notFound);

    server.begin();

  } else {
    Serial.println("Entering normal mode ...");
    initTime();
    pinMode(ledWake, OUTPUT);
    pinMode(ledSleep, OUTPUT);

    disconnectWiFi();  
  }
  
}

void loop() {
  if (setupMode) return;

  Serial.println((String)"Day :" + timeClient.getDay());
  Serial.println((String)"Hours :" + timeClient.getHours());
  Serial.println((String)"Minutes :" + timeClient.getMinutes());
  Serial.println((String)"Seconds :" + timeClient.getSeconds());
  Serial.println((String)"is time ? :" + isItTimeForWakeUp());
  Serial.println((String)"---------");
  

  if (isItTimeForWakeUp()) {
    digitalWrite(ledWake, HIGH);
    digitalWrite(ledSleep, LOW);
  } else {
    digitalWrite(ledWake, LOW);
    digitalWrite(ledSleep, HIGH);
  }

  delay(1000);
  
}