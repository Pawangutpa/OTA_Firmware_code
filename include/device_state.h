#pragma once
#include <Arduino.h>

struct DeviceState {
  bool ledState;
  String firmwareVersion;
};

extern DeviceState deviceState;
