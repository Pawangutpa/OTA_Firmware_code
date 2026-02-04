#include "ota_manager.h"
#include "mqtt_client.h"
#include <HTTPClient.h>
#include <Update.h>

extern PubSubClient mqtt;
extern String baseTopic;

void handleOtaCommand(String payload) {
  HTTPClient http;
  WiFiClient client;

  Serial.println("⬇️ OTA started");
  http.begin(client, payload);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("❌ OTA HTTP failed");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    http.end();
    return;
  }

  if (!Update.begin(contentLength)) {
    Serial.println("❌ Update.begin failed");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    http.end();
    return;
  }

  size_t written = Update.writeStream(*http.getStreamPtr());
  if (written != contentLength) {
    Serial.println("❌ OTA write incomplete");
    Update.abort();
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    http.end();
    return;
  }

  if (Update.end(true)) {
    Serial.println("✅ OTA success");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "SUCCESS");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("❌ OTA end failed");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
  }

  http.end();
}
