# ESP32 IoT Firmware

Industry-grade ESP32 firmware with:
- MQTT communication
- OTA updates with rollback
- Health monitoring
- PlatformIO & Arduino support

## Features
✔ Button → LED control  
✔ MQTT health reporting  
✔ OTA update via signed S3 URL  
✔ Rollback safe  
✔ CI enforced build  

## Supported Boards
- ESP32-S3

## Quick Start (PlatformIO)
```bash
git clone https://github.com/your-org/esp32-iot-firmware
cd esp32-iot-firmware
pio run
