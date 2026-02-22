#include "ota_manager.h"
#include "mqtt_client.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

extern PubSubClient mqtt;
extern String baseTopic;

void handleOtaCommand(String payload) {

  Serial.println("=================================");
  Serial.println("⬇️ OTA Command Received");
  Serial.println("URL:");
  Serial.println(payload);
  Serial.println("=================================");

  // 🔥 IMPORTANT: Use secure client for HTTPS
  WiFiClientSecure client;
  client.setInsecure();   // Required for AWS S3 signed URL

  HTTPClient https;

  if (!https.begin(client, payload)) {
    Serial.println("❌ HTTPS begin failed");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    return;
  }

  int httpCode = https.GET();

  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK) {
    Serial.println("❌ OTA HTTP request failed");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  int contentLength = https.getSize();

  Serial.print("Content Length: ");
  Serial.println(contentLength);

  if (contentLength <= 0) {
    Serial.println("❌ Invalid firmware size");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  if (!Update.begin(contentLength)) {
    Serial.println("❌ Update.begin failed");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  WiFiClient *stream = https.getStreamPtr();

  size_t written = Update.writeStream(*stream);

  Serial.print("Bytes Written: ");
  Serial.println(written);

  if (written != contentLength) {
    Serial.println("❌ OTA write incomplete");
    Update.abort();
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  if (Update.end(true)) {
    Serial.println("✅ OTA SUCCESS — Rebooting...");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "SUCCESS");
    delay(1000);
    ESP.restart();
  } else {
    Serial.print("❌ OTA End failed. Error #: ");
    Serial.println(Update.getError());
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
  }

  https.end();
}