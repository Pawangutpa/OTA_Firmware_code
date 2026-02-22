#include <Arduino.h>
#include <WiFi.h>
#include <device_state.h>
#include "config.h"
#include "mqtt_client.h"

unsigned long lastHealth = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(2,OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  mqttInit();
  pinMode(2, OUTPUT);
}

void loop() {
  mqttLoop();

  if (digitalRead(BUTTON_PIN) == LOW) {
    deviceState.ledState = !deviceState.ledState;
    digitalWrite(LED_PIN, deviceState.ledState);
    mqttPublishStatus();
    delay(300);
  }

  if (millis() - lastHealth > HEALTH_INTERVAL) {
    mqttPublishHealth();
    lastHealth = millis();
  }
  digitalWrite(2, HIGH);
  delay(50);
  digitalWrite(2, LOW);
  delay(10);
}
