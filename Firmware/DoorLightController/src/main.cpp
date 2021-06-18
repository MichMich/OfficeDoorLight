#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"
#include <ESP8266mDNS.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <ArduinoJson.h>

#define PIN_RED 12
#define PIN_ORANGE 13
#define PIN_GREEN 14

ESP8266WebServer server(80);

bool stateRed = false;
bool stateOrange = false;
bool stateGreen = false;

String getCurrentState() {
  StaticJsonDocument<200> doc;

  doc["red"] = stateRed;
  doc["orange"] = stateOrange;
  doc["green"] = stateGreen;

  String json;
  serializeJson(doc, json);
  return json;
}

void updateLeds() {
  digitalWrite(PIN_RED, stateRed);
  digitalWrite(PIN_ORANGE, stateOrange);
  digitalWrite(PIN_GREEN, stateGreen);
}

void setState(bool red, bool orange, bool green) {
  stateRed = red;
  stateOrange = orange;
  stateGreen = green;
  updateLeds();
}

void handleGetRoot() {
  String state = getCurrentState();

  Serial.println(state);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", state);
}

void handlePostRoot() {
  StaticJsonDocument<200> json;
  deserializeJson(json, server.arg("plain"));

  setState(json["red"], json["orange"], json["green"]);

  handleGetRoot();
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void configModeCallback (WiFiManager *myWiFiManager) {
  setState(false, true, true);
  Serial.println("Entered config mode ... Please connect to the config portal!");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {


  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_ORANGE, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);

  updateLeds();

  setState(true, true, true);

  Serial.begin(115200);

  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);

  if(!wifiManager.autoConnect("DoorLightController")) {
    setState(true, false, false);

    Serial.println("Failed to connect! Restarting ...");

    delay(1000);
    ESP.reset();
  }

  setState(false, true, false);
  Serial.print("Connected! " );
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleGetRoot);
  server.on("/", HTTP_POST, handlePostRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  if (!MDNS.begin("doorlight")) {
    Serial.println("Error setting up mDNS responder!");
  }
  Serial.println("mDNS responder started: doorlight.local");

  ArduinoOTA.begin();

  setState(false, false, false);
}



void loop() {
  server.handleClient();
  ArduinoOTA.handle();
}