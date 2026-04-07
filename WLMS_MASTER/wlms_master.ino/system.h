#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include "config.h"

struct SystemState {
    uint8_t level;
    uint16_t voltage;
    bool motor;
    bool isDay;
    bool isMainsCut;
    bool isWifiConnected;
    Mode mode;
    State state;

    int start_th;
    int stop_th;

    uint32_t motorStartTime;
    uint32_t lastLevelUpdate;

    bool fault;

    uint32_t dryRunLockUntil;
    uint32_t lastDryCheckTime;
    uint8_t lastDryCheckLevel;
    uint8_t dryRunRetries;
    uint32_t lastRetryTime;
};

struct SensorPacket {
    uint8_t level;
    int8_t temp;
    uint8_t checksum;
};

// 🔥 ONLY DECLARE here (no memory allocation)
extern SystemState sys;
extern SemaphoreHandle_t sysMutex;
extern SemaphoreHandle_t spiMutex;
extern SemaphoreHandle_t i2cMutex;
extern QueueHandle_t logQueue;