#include "device_identity.h"
#include <WiFi.h>

// Device ID = WiFi station MAC in standard byte order, uppercase, no colons
// e.g. "58E6C55A8204". This matches WiFi.macAddress() and the MAC you register
// in the webapp / provision on the broker (user "esp32_<MAC>").
//
// NOTE: the previous ESP.getEfuseMac() + "%04X%08X" approach printed the bytes
// REVERSED (e.g. "04825AC5E658"), which did not match the registered MAC and
// made the broker reject the device (esp32_<reversed> not found -> state 5).
String getDeviceMac() {
    String mac = WiFi.macAddress();  // "58:E6:C5:5A:82:04"
    mac.replace(":", "");
    mac.toUpperCase();
    return mac;                       // "58E6C55A8204"
}
