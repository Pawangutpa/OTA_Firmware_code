#include "mqtt_client.h"
#include "config.h"
#include "device_state.h"
#include "ota_manager.h"
#include <WiFi.h>
#include "device_identity.h"
WiFiClient espClient;
PubSubClient mqtt(espClient);

String baseTopic;


void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (int i = 0; i < len; i++) msg += (char)payload[i];

  if (String(topic).endsWith("/command")) {
    if (msg == "LED_ON") {
  deviceState.ledState = true;
  digitalWrite(LED_PIN, HIGH);
}

if (msg == "LED_OFF") {
  deviceState.ledState = false;
  digitalWrite(LED_PIN, LOW);
}

  }

  if (String(topic).endsWith("/ota")) {
    handleOtaCommand(msg);
  }
}

String normalizeMac(String mac) {
  mac.replace(":", "");
  return mac;
}


void mqttInit() {
  baseTopic = "devices/" + getEfuseMacString();
  Serial.println("Device MAC: " + baseTopic);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
}


void mqttReconnect() {
  static unsigned long lastAttempt = 0;

  if (millis() - lastAttempt < 3000) return;
  lastAttempt = millis();

  String efuseMac = getEfuseMacString();

  String clientId = "esp32_" + efuseMac;
  String mqttUser = "esp32_" + efuseMac;
  String mqttPass = efuseMac;

  if (mqtt.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    mqtt.subscribe((baseTopic + "/command").c_str());
    mqtt.subscribe((baseTopic + "/ota").c_str());
  }
}


void mqttLoop() {
  if (!mqtt.connected()) {
    mqttReconnect();
  }
  mqtt.loop();
}


void mqttPublishStatus() {
  mqtt.publish((baseTopic + "/status").c_str(),
               deviceState.ledState ? "ON" : "OFF");
}

void mqttPublishHealth() {
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"heap\":%d,\"uptime\":%lu,\"fw\":\"%s\"}",
           ESP.getFreeHeap(),
           millis() / 1000,
           FW_VERSION);
  mqtt.publish((baseTopic + "/health").c_str(), payload);
}
