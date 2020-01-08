#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

AsyncWebServer server(80);

const char* directSsid = "SleeperConf";

const int ledSleepPin    = 25;
const int ledWakePin     = 27;
const int setupButtonPin = 32;

bool setupMode      = false;
bool credentialMode = false;

DynamicJsonDocument jsonPlanning(8176);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "mafreebox.free.fr");

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

bool connectWiFi()
{
    StaticJsonDocument<1024> jsonSetup;
    
    File setupFile = SPIFFS.open("/setup.json", FILE_READ );
    DeserializationError err = deserializeJson(jsonSetup, setupFile);
    setupFile.close();
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
      return false;
    }
    
    const char* ssid = jsonSetup["ssid"];
    const char* password = jsonSetup["password"];
    uint retryCount = 0;

    Serial.println((String) "Connecting to " + ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && retryCount < 3) {
        retryCount++;
        delay(500);
        Serial.print(".");
    }
    return WiFi.status() == WL_CONNECTED;
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

bool enterSetupMode()
{
  pinMode(setupButtonPin,INPUT_PULLUP);
  return !digitalRead(setupButtonPin);
}

void initDirectWifi()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(directSsid);
}

void setupWebServer()
{
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
}

void setup() {
  Serial.begin(9600);

  if (!SPIFFS.begin()) {
    Serial.println("ERROR while initializing SPIFFS");
    return;
  }

  if (!connectWiFi()) {
    Serial.println("Entering CREDENTIALS MODE ...");
    Serial.println("Please connect to http://192.168.4.1");
    credentialMode = true;
    initDirectWifi();
    return;
  }

  // Print local IP address and start web server
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (enterSetupMode()) {
    Serial.println("Entering SETUP MODE ...");
    setupMode = true;

    MDNS.begin("sleeper");
    setupWebServer();
    MDNS.addService("http", "tcp", 80);

  } else {
    Serial.println("Entering normal mode ...");

    loadPlanningFile();
    connectWiFi();
    
    initTime();
    pinMode(ledWakePin, OUTPUT);
    pinMode(ledSleepPin, OUTPUT);

    disconnectWiFi();
  }
  
}

void loop() {
  if (setupMode || credentialMode) return;

  Serial.println((String)"Day :" + timeClient.getDay());
  Serial.println((String)"Hours :" + timeClient.getHours());
  Serial.println((String)"Minutes :" + timeClient.getMinutes());
  Serial.println((String)"Seconds :" + timeClient.getSeconds());
  Serial.println((String)"is time ? :" + isItTimeForWakeUp());
  Serial.println((String)"---------");
  

  if (isItTimeForWakeUp()) {
    digitalWrite(ledWakePin, HIGH);
    digitalWrite(ledSleepPin, LOW);
  } else {
    digitalWrite(ledWakePin, LOW);
    digitalWrite(ledSleepPin, HIGH);
  }

  delay(1000);
  
}