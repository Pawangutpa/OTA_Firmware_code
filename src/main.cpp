#include <Arduino.h>
#include <WiFi.h>
#include <device_state.h>
#include "config.h"
#include "mqtt_client.h"
#include "ota_manager.h"

unsigned long lastHealth = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(2, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Bounded wait: a bad OTA image that can't join WiFi must still reach loop()
  // so otaVerifyLoop() can roll it back instead of hanging here forever.
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 20000) {
    delay(500);
    Serial.print(".");  // progress dots while connecting to WiFi
  }

  mqttInit();
  otaInit();  // if this is a post-OTA trial boot, start verifying the new image
}

void loop() {
  mqttLoop();
  otaVerifyLoop();  // commit the new firmware, or roll back if it can't connect

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
  Serial.println("Blinking LED on pin 2 version " FW_VERSION);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(200);
}
