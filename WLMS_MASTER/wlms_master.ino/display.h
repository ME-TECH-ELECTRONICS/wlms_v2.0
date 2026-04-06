#pragma once

#include <stdint.h>

void initDisplay();
void welcomeScreen();
void updateDisplay(uint8_t level, uint16_t voltage, uint16_t volume, const char* dateTime, bool isMotorOn = false, bool isWifiConnected = false, bool isMainsCut = false);