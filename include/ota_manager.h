#pragma once
#include <Arduino.h>

// Download + flash firmware from the signed URL published by the backend.
void handleOtaCommand(String payload);

// Call once in setup(): detects a post-OTA "trial" boot and arms verification.
void otaInit();

// Call every loop(): commits the new firmware once it proves healthy, or rolls
// back to the previous partition if it can't connect within the timeout.
void otaVerifyLoop();
