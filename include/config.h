#pragma once

// ================= WiFi =================
#define WIFI_SSID       "One Plus 11R"
#define WIFI_PASSWORD   "shivam12345"

// ================= MQTT =================
// Must be the SAME broker the backend/frontend use (15.206.91.29).
// Device uses native MQTT on 1883; backend uses WebSocket on 9001 of the
// same Mosquitto broker. Ensure EC2 security group allows inbound 1883.
#define MQTT_BROKER     "15.206.91.29"
#define MQTT_PORT       1883
// The device does NOT use a fixed user/pass. It authenticates per-device with
// credentials derived from its eFuse MAC (user "esp32_<MAC>", pass "<MAC>") -
// the same ones the backend assigns on registration. See mqtt_client.cpp.
#define MQTT_USER       ""
#define MQTT_PASS       ""

// ================= DEVICE =================
#define DEVICE_TYPE     "esp32s3"
#define HEALTH_INTERVAL 10000  // ms

// ================= GPIO =================
#define LED_PIN         4
#define BUTTON_PIN      5
