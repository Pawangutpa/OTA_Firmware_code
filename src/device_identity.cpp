#include "device_identity.h"

String getEfuseMacString() {
    uint64_t chipid = ESP.getEfuseMac();

    char macStr[13];  // 12 hex chars + null
    sprintf(macStr, "%04X%08X",
            (uint16_t)(chipid >> 32),
            (uint32_t)chipid);

    return String(macStr);
}