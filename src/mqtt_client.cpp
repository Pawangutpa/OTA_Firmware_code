#include "mqtt_client.h"
#include "config.h"
#include "device_state.h"
#include "ota_manager.h"
#include <WiFi.h>

WiFiClient espClient;
PubSubClient mqtt(espClient);

String baseTopic;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (int i = 0; i < len; i++) msg += (char)payload[i];

  if (String(topic).endsWith("/command")) {
    if (msg == "LED_ON") digitalWrite(LED_PIN, HIGH);
    if (msg == "LED_OFF") digitalWrite(LED_PIN, LOW);
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
  baseTopic = "devices/" + normalizeMac(WiFi.macAddress());
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
}


void mqttReconnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect(WiFi.macAddress().c_str(), MQTT_USER, MQTT_PASS)) {
      mqtt.subscribe((baseTopic + "/command").c_str());
      mqtt.subscribe((baseTopic + "/ota").c_str());
    } else {
      delay(2000);
    }
  }
}

void mqttLoop() {
  if (!mqtt.connected()) mqttReconnect();
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
