#pragma once

#include <Arduino.h>

constexpr uint32_t SETTINGS_VERSION = 1;

struct Settings {
    uint8_t motorStartThreshold;
    uint8_t motorStopThreshold;

    uint8_t dayStartHour;
    uint8_t dayEndHour;

    uint16_t voltageMin;
    uint16_t voltageMax;
    uint16_t voltageFail;

    uint32_t dryRunLockMs;
    uint32_t dryRunIntervalMs;
    uint8_t dryRunMinIncrease;
    uint8_t dryRunMaxRetries;
    uint32_t dryRunStartupDelayMs;

    uint32_t sensorTimeoutMs;
    uint32_t maxMotorRuntimeMs;
    uint32_t faultRecoveryTimeMs;

    uint32_t retryDelayMs;
};

extern Settings settings;
extern const Settings factorySettings;

bool loadSettings();
bool saveSettings();
void factoryReset();