
---

# 📄 `docs/OTA.md`

```md
# OTA (Over-The-Air) Update System

This document explains the complete OTA firmware update system used
for ESP32 devices in this project.

It covers:
- Firmware storage
- Backend decision logic
- MQTT OTA trigger
- ESP32 update process
- Rollback safety
- CI/CD integration

---

## 1. OTA Overview

OTA allows firmware updates **without physical access** to devices.

The OTA system consists of:
- AWS S3 (firmware storage)
- Backend server (control & security)
- MQTT (trigger delivery)
- ESP32 dual-partition firmware

---

## 2. Firmware Storage Structure (S3)

Bucket:
sds-pawan-testing


### 2.1 Production Firmware



esp32s3/production/
├── firmware.bin
├── version.txt
└── metadata.json


This folder contains **only the currently deployed firmware**.

---

### 2.2 Release History



esp32s3/releases/
├── v1.0.0/
├── v1.1.0/
└── v1.2.0/


Each version folder contains:
- firmware.bin
- version.txt
- metadata.json

These are **never deleted**.

---

### 2.3 Rollback Rules



esp32s3/rollback/
└── blocked_versions.json


Example:
```json
{
  "blocked_versions": ["1.1.0"],
  "reason": {
    "1.1.0": "Boot loop after OTA"
  }
}

3. OTA Decision Flow (Backend)

Device reports firmware version via MQTT

User clicks Check Update in dashboard

Backend reads:

esp32s3/production/version.txt


Backend compares versions

Backend checks blocked_versions.json

If allowed:

Backend generates signed S3 URL

Backend publishes OTA trigger

4. OTA Trigger via MQTT

Topic

devices/{DEVICE_ID}/ota


Payload

<SIGNED_S3_URL>


Example:

https://sds-pawan-testing.s3.ap-south-1.amazonaws.com/esp32s3/production/firmware.bin?...expires=300


Signed URLs:

Expire automatically

Require no AWS credentials on device

5. ESP32 OTA Execution
Steps on Device

ESP32 receives MQTT OTA message

Downloads firmware using HTTP

Writes firmware to inactive OTA partition

Verifies write completion

Reboots device

6. OTA Status Feedback

After update attempt, ESP32 publishes:

Topic

devices/{DEVICE_ID}/ota/status


Payloads

SUCCESS
FAILED


Backend updates:

Device firmware version

OTA history

Dashboard status

7. Rollback & Safety
Automatic Rollback

ESP32 uses dual OTA partitions.

If:

Power loss during OTA

Firmware crash after reboot

Corrupted firmware

➡ Device automatically boots previous firmware.

No manual intervention required.

8. CI/CD Integration

Firmware is deployed automatically using GitHub Actions:

Code pushed to main

PlatformIO builds firmware

Firmware uploaded to:

releases/vX.Y.Z/

Firmware promoted to:

production/

Backend detects new version

OTA becomes available

9. Failure Scenarios & Handling
Scenario	Outcome
Device offline	OTA not triggered
Network loss	OTA fails, old firmware stays
Power failure	Automatic rollback
Bad firmware	Blocked via rollback file