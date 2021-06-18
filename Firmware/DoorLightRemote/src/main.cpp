#include <Arduino.h>

#include "WiFiManager.h"

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <AceButton.h>
#include <ESP8266HTTPClient.h>
#include <elapsedMillis.h>

using namespace ace_button;


#define LED_RED D5
#define LED_ORANGE D6
#define LED_GREEN D7

#define BUTTON_RED D1
#define BUTTON_ORANGE D2
#define BUTTON_GREEN D3

#define BRIGHTNESS_OFF 32
#define BRIGHTNESS_ON 1023

AceButton buttonRed(BUTTON_RED);
AceButton buttonOrange(BUTTON_ORANGE);
AceButton buttonGreen(BUTTON_GREEN);

bool stateRed = false;
bool stateOrange = false;
bool stateGreen = false;

WiFiClient client;
HTTPClient http;

elapsedMillis updateTimer;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode ... Please connect to the config portal!");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

String getStateJson(bool stateRed, bool stateOrange, bool stateGreen) {
  StaticJsonDocument<200> doc;

  doc["red"] = stateRed;
  doc["orange"] = stateOrange;
  doc["green"] = stateGreen;

  String json;
  serializeJson(doc, json);
  return json;
}

void updateLeds() {
  analogWrite(LED_RED, stateRed ? BRIGHTNESS_ON : BRIGHTNESS_OFF);
  analogWrite(LED_ORANGE, stateOrange ? BRIGHTNESS_ON : BRIGHTNESS_OFF);
  analogWrite(LED_GREEN, stateGreen ? BRIGHTNESS_ON : BRIGHTNESS_OFF);

  // digitalWrite(LED_RED, stateRed);
  // digitalWrite(LED_ORANGE, stateOrange);
  // digitalWrite(LED_GREEN, stateGreen);
}

void updateState(String stateJson) {
  StaticJsonDocument<200> json;
  deserializeJson(json, stateJson);

  stateRed = json["red"];
  stateOrange = json["orange"];
  stateGreen = json["green"];

  updateLeds();
}

void sendStateUpdate(String stateJson) {

  // Serial.println(stateJson);


  http.begin(client, "http://192.168.1.123/");
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(stateJson);
  String payload = http.getString();

  // Serial.println(httpCode);
  // Serial.println(payload);

  http.end();

  updateState(payload);
}




void toggleRed() {
  sendStateUpdate(getStateJson(!stateRed, false, false));
}

void toggleOrange() {
  sendStateUpdate(getStateJson(false, !stateOrange, false));
}

void toggleGreen() {
  sendStateUpdate(getStateJson(false, false, !stateGreen));
}

void toggleOff() {
  sendStateUpdate(getStateJson(false, false, false));
}

void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventClicked:
      if (button->getPin() == BUTTON_RED) toggleRed();
      if (button->getPin() == BUTTON_ORANGE) toggleOrange();
      if (button->getPin() == BUTTON_GREEN) toggleGreen();

    break;
    case AceButton::kEventLongPressed:
      toggleOff();
    break;
  }

  // Serial.print(F("handleEvent(): pin: "));
  // Serial.print(button->getPin());
  // Serial.print(F("; eventType: "));
  // Serial.print(eventType);
  // Serial.print(F("; buttonState: "));
  // Serial.println(buttonState);
}

void fetchState() {
  http.begin(client, "http://192.168.1.123/");

  int httpCode = http.GET();
  String payload = http.getString();

  // Serial.println(httpCode);
  // Serial.println(payload);

  http.end();

  updateState(payload);
}


void setup() {
  Serial.begin(9600);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_ORANGE, HIGH);
  digitalWrite(LED_GREEN, HIGH);

  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_ORANGE, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);

  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);

  if(!wifiManager.autoConnect("DoorLightRemote")) {
    Serial.println("Failed to connect! Restarting ...");

    delay(1000);
    ESP.reset();
  }

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setClickDelay(500);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);

  ArduinoOTA.begin();

  updateLeds();
}

void loop() {
  buttonRed.check();
  buttonOrange.check();
  buttonGreen.check();

  if (updateTimer > 1000) {
    fetchState();
    updateTimer = 0;
  }

  ArduinoOTA.handle();
}

