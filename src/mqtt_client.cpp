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

  Serial.println("\n====== MQTT CALLBACK ======");
  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Payload length: ");
  Serial.println(len);

  String msg;
  for (int i = 0; i < len; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Payload: ");
  Serial.println(msg);
  Serial.println("===========================\n");

  String t = String(topic);

  if (t.endsWith("/ota")) {
    Serial.println("OTA TOPIC MATCHED");
    handleOtaCommand(msg);
    return;
  }

  // Backend publishes "LED_ON" / "LED_OFF" on devices/{id}/command
  if (t.endsWith("/command")) {
    if (msg == "LED_ON") {
      deviceState.ledState = true;
      digitalWrite(LED_PIN, HIGH);
      mqttPublishStatus();
    } else if (msg == "LED_OFF") {
      deviceState.ledState = false;
      digitalWrite(LED_PIN, LOW);
      mqttPublishStatus();
    }
    return;
  }
}

String normalizeMac(String mac) {
  mac.replace(":", "");
  return mac;
}


void mqttInit() {
  baseTopic = "devices/" + getDeviceMac();
  Serial.println("Device MAC: " + baseTopic);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
}


// Decode PubSubClient's mqtt.state() into a human-readable reason.
static const char *mqttStateToString(int state) {
  switch (state) {
    case -4: return "CONNECTION_TIMEOUT (broker unreachable / no response)";
    case -3: return "CONNECTION_LOST (TCP dropped)";
    case -2: return "CONNECT_FAILED (can't reach broker - IP/port/firewall)";
    case -1: return "DISCONNECTED";
    case  0: return "CONNECTED";
    case  1: return "BAD_PROTOCOL";
    case  2: return "BAD_CLIENT_ID";
    case  3: return "SERVER_UNAVAILABLE";
    case  4: return "BAD_CREDENTIALS (wrong user/pass)";
    case  5: return "UNAUTHORIZED (rejected by broker/ACL)";
    default: return "UNKNOWN";
  }
}

void mqttReconnect() {
  static unsigned long lastAttempt = 0;

  if (millis() - lastAttempt < 3000) return;
  lastAttempt = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[MQTT] Waiting - WiFi not connected yet");
    return;
  }

  // Per-device credentials. The backend creates a Mosquitto user
  // "esp32_<MAC>" with password "<MAC>" when the device is registered
  // (backend acl.service.addMqttUser). Authenticate with exactly that.
  String mac = getDeviceMac();
  String clientId = "esp32_" + mac;
  String mqttUser = "esp32_" + mac;
  String mqttPass = mac;

  Serial.printf("[MQTT] Connecting to %s:%d as '%s' ... ",
                MQTT_BROKER, MQTT_PORT, mqttUser.c_str());

  if (mqtt.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    Serial.println("OK");
    mqtt.subscribe((baseTopic + "/command").c_str());
    mqtt.subscribe((baseTopic + "/ota").c_str());
    Serial.println("[MQTT] Subscribed: " + baseTopic + "/command , " +
                   baseTopic + "/ota");
  } else {
    int st = mqtt.state();
    Serial.printf("FAILED\n[MQTT] Not connected: state=%d (%s). Retry in 3s.\n",
                  st, mqttStateToString(st));
  }
}


void mqttLoop() {
  static bool wasConnected = false;

  if (mqtt.connected()) {
    if (!wasConnected) {
      Serial.println("[MQTT] Link up");
      wasConnected = true;
    }
  } else {
    if (wasConnected) {
      int st = mqtt.state();
      Serial.printf("[MQTT] Link DOWN: state=%d (%s)\n",
                    st, mqttStateToString(st));
      wasConnected = false;
    }
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
