#include "ota_manager.h"
#include "mqtt_client.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>

extern PubSubClient mqtt;
extern String baseTopic;

// How long a freshly-flashed image has to connect to the MQTT broker before we
// treat it as broken and revert to the previous partition.
#ifndef OTA_VERIFY_TIMEOUT_MS
#define OTA_VERIFY_TIMEOUT_MS 90000UL  // 90 s
#endif

static bool otaTrialActive = false;
static unsigned long otaTrialStart = 0;

// --- trial flag persisted in NVS (survives the reboot into the new image) ---
static void otaSetTrialFlag(bool v) {
  Preferences p;
  p.begin("ota", false);
  p.putBool("trial", v);
  p.end();
}

static bool otaGetTrialFlag() {
  Preferences p;
  p.begin("ota", true);
  bool v = p.getBool("trial", false);
  p.end();
  return v;
}

void handleOtaCommand(String payload) {
  Serial.println("\n==============================");
  Serial.println("OTA PROCESS START");
  Serial.println("==============================");
  Serial.print("Free Heap Before OTA: ");
  Serial.println(ESP.getFreeHeap());

  // Proof that the existing firmware is preserved: the new image is written to
  // the OTHER (inactive) partition, never over the one currently running.
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
  Serial.printf("[OTA] Keeping current firmware in '%s'; writing new image to '%s'\n",
                running ? running->label : "?",
                target ? target->label : "?");

  Serial.println("OTA URL:");
  Serial.println(payload);

  WiFiClientSecure client;
  client.setInsecure();  // S3 presigned HTTPS URL; skip cert-chain validation

  HTTPClient https;
  // Bound slow/stalled networks: a hung or dropped download fails cleanly and
  // the device keeps running the CURRENT firmware (the running partition is
  // never touched during download).
  https.setConnectTimeout(10000);  // ms to establish the connection
  https.setTimeout(15000);         // ms with no data before the read aborts
  if (!https.begin(client, payload)) {
    Serial.println("https.begin() FAILED");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    return;
  }

  int httpCode = https.GET();
  Serial.print("HTTP Code: ");
  Serial.println(httpCode);
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("HTTP GET FAILED");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  int contentLength = https.getSize();
  Serial.print("Firmware Size: ");
  Serial.println(contentLength);
  if (contentLength <= 0) {
    Serial.println("Invalid firmware size");
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  if (!Update.begin(contentLength)) {
    Serial.print("Update.begin FAILED. Error: ");
    Serial.println(Update.getError());
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  // Let the backend know we started downloading/flashing.
  mqtt.publish((baseTopic + "/ota/status").c_str(), "IN_PROGRESS");

  Update.onProgress([](size_t done, size_t total) {
    static int lastPct = -1;
    int pct = total ? (int)((done * 100) / total) : 0;
    if (pct != lastPct && (pct % 10 == 0)) {
      Serial.printf("[OTA] %d%%\n", pct);
      lastPct = pct;
    }
  });

  WiFiClient *stream = https.getStreamPtr();
  size_t written = Update.writeStream(*stream);
  Serial.print("Bytes Written: ");
  Serial.println(written);

  if (written != (size_t)contentLength) {
    Serial.println("Firmware write incomplete");
    Update.abort();
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
    return;
  }

  // Update.end(true) verifies the image checksum and sets the new slot as boot.
  // A corrupt download is rejected here (won't be booted).
  if (Update.end(true)) {
    Serial.println("Firmware written OK. Booting new image on trial...");
    https.end();
    // Mark the new image UNVERIFIED. After reboot the new firmware must prove
    // it can connect (otaVerifyLoop) or we roll back to the old partition.
    // NOTE: we deliberately do NOT publish SUCCESS yet — success is only
    // reported once the new firmware is confirmed healthy after reboot.
    otaSetTrialFlag(true);
    delay(1500);
    ESP.restart();
  } else {
    Serial.print("Update.end FAILED. Error: ");
    Serial.println(Update.getError());
    mqtt.publish((baseTopic + "/ota/status").c_str(), "FAILED");
    https.end();
  }
}

void otaInit() {
  // Create the NVS namespace on first boot so the read below doesn't log a
  // harmless "nvs_open failed: NOT_FOUND" the very first time.
  { Preferences p; p.begin("ota", false); p.end(); }

  otaTrialActive = otaGetTrialFlag();
  if (otaTrialActive) {
    otaTrialStart = millis();
    Serial.println("[OTA] Trial boot detected - verifying new firmware...");
  }
}

void otaVerifyLoop() {
  if (!otaTrialActive) return;

  // Healthy: the new firmware reached the broker -> commit it permanently.
  if (mqtt.connected()) {
    Serial.println("[OTA] New firmware verified healthy -> COMMIT");
    esp_ota_mark_app_valid_cancel_rollback();  // no-op if bootloader rollback off
    otaSetTrialFlag(false);
    otaTrialActive = false;
    mqtt.publish((baseTopic + "/ota/status").c_str(), "SUCCESS");
    return;
  }

  // Unhealthy for too long: revert to the previous (known-good) partition.
  if (millis() - otaTrialStart > OTA_VERIFY_TIMEOUT_MS) {
    Serial.println("[OTA] New firmware FAILED to verify -> ROLLING BACK");
    otaSetTrialFlag(false);
    const esp_partition_t *prev = esp_ota_get_next_update_partition(NULL);
    if (prev && esp_ota_set_boot_partition(prev) == ESP_OK) {
      Serial.println("[OTA] Reverted to previous partition. Rebooting...");
    } else {
      Serial.println("[OTA] Rollback FAILED (could not set boot partition)");
    }
    delay(1000);
    ESP.restart();
  }
}
