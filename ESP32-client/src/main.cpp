#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFiMulti.h>

#include "config.h"

#define WS_HOST "k6dije477f.execute-api.eu-north-1.amazonaws.com"
#define WS_PORT 443
#define WS_URL "/dev"

#define MSG_SIZE 256

WiFiMulti wifiMulti;
WebSocketsClient wsClient;

const int LedPin = 4;

void sendErrorMessage(const char* error) {
  char msg[MSG_SIZE];

  sprintf(msg, "{\"action\":\"msg\",\"type\":\"error\",\"body\":\"%s\"}}", error);

  wsClient.sendTXT(msg);
}

void sendOkMessage() {
  wsClient.sendTXT("{\"action\":\"msg\",\"type\":\"status\",\"body\":\"ok\"}}");
}

uint8_t toMode(const char* val) {
  if (strcmp(val, "output") == 0) {
    return OUTPUT;
  }

  if (strcmp(val, "input_pullup") == 0) {
    return INPUT_PULLUP;
  }

  return INPUT;
}

void HandleMessage(uint8_t* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());

    sendErrorMessage(error.c_str());

    return;
  }

  if (!doc["type"].is<const char*>()) {
    sendErrorMessage("invalid message type format");
    return;
  }

  if (strcmp(doc["type"], "cmd") == 0) {
    if (!doc["body"].is<JsonObject>()) {
      sendErrorMessage("invalid command body");
      return;
    }

    if (strcmp(doc["body"]["type"], "pinMode") == 0) {
      pinMode(doc["body"]["pin"], toMode(doc["body"]["mode"]));
      sendOkMessage();
      return;
    }

    if (strcmp(doc["body"]["type"], "digitalWrite") == 0) {
      pinMode(doc["body"]["pin"], toMode(doc["body"]["mode"]));
      sendOkMessage();
      return;
    }

    if (strcmp(doc["body"]["type"], "digitalRead") == 0) {
      auto value = digitalRead(doc["body"]["pin"]);

      char msg[MSG_SIZE];

      sprintf(msg, "{\"action\":\"msg\",\"type\":\"output\",\"body\":\"%d\"}}", value);

      wsClient.sendTXT(msg);

      return;
    }

    sendErrorMessage("unsupported command type");
    return;
  }

  sendErrorMessage("unsupported message type");
  return;
}

void onWSEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("WS Connected");
      break;

    case WStype_DISCONNECTED:
      Serial.println("WS Disconnected");
      break;

    case WStype_TEXT:
      Serial.printf("WS Message: %s\n", payload);
      HandleMessage(payload);
      break;
  }
}

void setup() {
  Serial.begin(921600);
  pinMode(LedPin, OUTPUT);

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println("Connected to wifi");

  wsClient.beginSSL(WS_HOST, WS_PORT, WS_URL, "", "wss");
  wsClient.onEvent(onWSEvent);
}

void loop() {
  digitalWrite(LedPin, WiFi.status() == WL_CONNECTED);
  wsClient.loop();
}