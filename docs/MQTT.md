# MQTT Architecture & Topic Design

This document explains the complete MQTT design used by the ESP32 firmware,
backend server, and Mosquitto broker.

It covers:
- Topic structure
- Payload formats
- Security (ACL)
- Dummy examples
- Real production flow

---

## 1. Overview

MQTT is used as the **real-time communication layer** between:

- ESP32 devices
- Backend server (Node.js)
- Web dashboard (via backend WebSocket)

The broker used is **Mosquitto**, hosted on the same EC2 instance as the backend.

---

## 2. Broker Configuration

| Item | Value |
|---|---|
| Broker | Mosquitto |
| MQTT Port | 1883 |
| WebSocket Port | 9001 |
| Auth | Username / Password |
| Anonymous | Disabled |

---

## 3. Device Identity Model

Each device is identified using its **MAC address without colons**.

### Example

ESP32 MAC Address : 0C:5D:32:DD:25:68
Device ID : 0C5D32DD2568

This `Device ID` is used:
- In MQTT topics
- In MongoDB
- In S3 OTA logic
- In ACL rules

---

## 4. Topic Naming Convention

All topics follow this strict pattern:

devices/{DEVICE_ID}/{topic}
### Example

devices/0C5D32DD2568/health


This ensures:
- Isolation between devices
- Easy ACL enforcement
- Predictable routing

---

## 5. Topics Used (FULL LIST)

### 5.1 Device → Backend Topics

| Topic | Purpose |
|---|---|
| `devices/{id}/health` | Periodic health report |
| `devices/{id}/status` | LED / relay status |
| `devices/{id}/ota/status` | OTA result feedback |

---

### 5.2 Backend → Device Topics

| Topic | Purpose |
|---|---|
| `devices/{id}/command` | Control commands |
| `devices/{id}/ota` | OTA trigger (signed URL) |

---

## 6. Payload Formats (WITH DUMMY DATA)

### 6.1 Health Payload

**Topic**

devices/0C5D32DD2568/health


**Payload**
```json
{
  "heap": 24320,
  "uptime": 360,
  "fw": "1.0.0"
}
Field	Meaning
heap	Free heap memory (bytes)
uptime	Seconds since boot
fw	Firmware version
6.2 Status Payload

Topic

devices/0C5D32DD2568/status


Payload

ON


or

OFF

6.3 Command Payload

Topic

devices/0C5D32DD2568/command


Payloads

LED_ON
LED_OFF

6.4 OTA Trigger Payload

Topic

devices/0C5D32DD2568/ota


Payload (Signed S3 URL)

https://sds-pawan-testing.s3.ap-south-1.amazonaws.com/esp32s3/production/firmware.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Expires=300&...


⚠️ Devices never access S3 without a signed URL.

6.5 OTA Status Payload

Topic

devices/0C5D32DD2568/ota/status


Payload

SUCCESS


or

FAILED

7. Security & ACL Model

Each device has:

A unique MQTT username

A unique password

Topic-level ACL restrictions

Example ACL (Mosquitto)
user esp32_0C5D32DD2568

topic write devices/0C5D32DD2568/health
topic write devices/0C5D32DD2568/status
topic write devices/0C5D32DD2568/ota/status

topic read  devices/0C5D32DD2568/command
topic read  devices/0C5D32DD2568/ota


Backend user:

user backend_server
topic readwrite devices/#

8. Device Blocking Behavior

When a device is blocked:

Backend removes its ACL entry

MQTT connection is dropped

Device cannot reconnect

No firmware change required on device

9. Design Rules (IMPORTANT)

✔ Devices never publish commands
✔ Backend never publishes health
✔ OTA only triggered by backend
✔ Topics are predictable and auditable

10. Summary

MQTT provides:

Real-time communication

Secure isolation per device

OTA trigger delivery

Health & status reporting

This design scales from 1 device to 1 million devices safely.