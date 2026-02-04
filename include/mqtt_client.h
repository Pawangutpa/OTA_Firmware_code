#pragma once
#include <PubSubClient.h>

void mqttInit();
void mqttLoop();
void mqttPublishStatus();
void mqttPublishHealth();
