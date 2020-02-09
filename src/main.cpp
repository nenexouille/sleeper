#include <main.h>


bool setupMode        = false;
bool shouldSaveConfig = false;

AsyncWebServer server(80);
DynamicJsonDocument jsonPlanning(8176);
ntp ntpClient;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void initTime()
{
    ntpClient.client->begin();
    ntpClient.client->setTimeOffset(3600);

    while(!ntpClient.client->update()) {
        ntpClient.client->forceUpdate();
    }
}

bool isItTimeForWakeUp()
{
  return((bool) jsonPlanning[ntpClient.client->getDay()][ntpClient.client->getHours()][ntpClient.client->getMinutes()<30?0:1]);
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
  pinMode(PIN_BUTTON_SETUP,INPUT_PULLUP);
  return !digitalRead(PIN_BUTTON_SETUP);
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
  
  Serial.begin(921600);
  btStop(); // disable bluetooth

  //WiFi.mode(WIFI_STA);

  if (SPIFFS.begin()) {
    Serial.println(F("\nSPIFFS mounted"));
  } else {
    Serial.println(F("\nFailed to mount SPIFFS"));
  }

  WiFiManagerParameter custom_ntp_server("ntp_server", "ntp server", NTP_SERVER, 255);
  WiFiManager wifiManager;

  //wifiManager.resetSettings();
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_ntp_server);
  
  if(wifiManager.autoConnect(WIFI_CONFIG_SSID)) {
      Serial.println("Failed to connect");
      // ESP.restart();
  }

  StaticJsonDocument<200> jsonConfig;

  if (shouldSaveConfig) {
    File configFile = SPIFFS.open("/config.json", FILE_WRITE);
    Serial.println("saving config");
    jsonConfig["ntp_server"] = (char*) custom_ntp_server.getValue();
    serializeJson(jsonConfig, configFile);
    configFile.close();
  }

  File configFile = SPIFFS.open("/config.json", FILE_READ);
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }
  deserializeJson(jsonConfig, configFile);
  configFile.close();


  // Print local IP address and start web server
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (enterSetupMode()) {
    Serial.println("Entering SETUP MODE ...");
    setupMode = true;

    MDNS.begin("sleeper");
    MDNS.addService("http", "tcp", 80);
    setupWebServer();
    

  } else {
    Serial.println("Entering normal mode ...");

    pinMode(PIN_LED_WAKE, OUTPUT);
    pinMode(PIN_LED_SLEEP, OUTPUT);

    WiFiUDP ntpUDP;
    ntpClient.client = new NTPClient (ntpUDP, jsonConfig["ntp_server"], 3600);
    
    loadPlanningFile();
    initTime();

    WiFi.mode(WIFI_OFF);
  }
  
}

void loop() {
  if (setupMode) return;

  Serial.println((String)"Day :" + ntpClient.client->getDay());
  Serial.println((String)"Hours :" + ntpClient.client->getHours());
  Serial.println((String)"Minutes :" + ntpClient.client->getMinutes());
  Serial.println((String)"Seconds :" + ntpClient.client->getSeconds());
  Serial.println((String)"is time ? :" + isItTimeForWakeUp());
  Serial.println((String)"---------");
  

  if (isItTimeForWakeUp()) {
    digitalWrite(PIN_LED_WAKE, HIGH);
    digitalWrite(PIN_LED_SLEEP, LOW);
  } else {
    digitalWrite(PIN_LED_WAKE, LOW);
    digitalWrite(PIN_LED_SLEEP, HIGH);
  }

  delay(1000);
  
}