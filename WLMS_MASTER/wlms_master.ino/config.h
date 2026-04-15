#include "driver/adc_types_legacy.h"
#include <stdint.h>
#pragma once

constexpr const char* VERSION = "1.0.0";

// Pins
constexpr uint8_t MOTOR = 15;
constexpr uint8_t MOTOR_STATUS_LED = 33;
constexpr uint8_t MODE_LED_RED = 25;
constexpr uint8_t MODE_LED_GREEN = 26;
constexpr uint8_t MODE_BTN = 27;
constexpr uint8_t MANUAL_BTN = 14;
constexpr adc1_channel_t VOLTAGE_SENS = ADC1_CHANNEL_6;
constexpr uint8_t LORA_RST = 16;
constexpr uint8_t LORA_D0 = 4;
constexpr uint8_t LORA_CHIP_SELECT = 5;
constexpr uint8_t SDCARD_CHIP_SELECT = 17;

// Thresholds
constexpr uint8_t MOTOR_START_THRESHOLD = 10;
constexpr uint8_t MOTOR_STOP_THRESHOLD = 100;

constexpr uint8_t DAY_START_HOUR = 1;
constexpr uint8_t DAY_END_HOUR = 23;

constexpr uint8_t VOLTAGE_MIN = 220;
constexpr uint8_t VOLTAGE_MAX = 250;
constexpr uint8_t VOLTAGE_FAIL = 110;

// Timing
constexpr uint32_t DRYRUN_CHECK_INTERVAL_MS = 15000;
constexpr uint8_t DRYRUN_MIN_INCREASE = 2;
constexpr uint8_t DRYRUN_MAX_RETRIES = 2;
constexpr uint32_t SENSOR_TIMEOUT_MS = 5000UL;
constexpr uint32_t MAX_MOTOR_RUNTIME_MS = (10UL * 60UL * 1000UL);

constexpr char const* ACCESS_POINT_SSID = "METECH";
constexpr char const* ACCESS_POINT_PASSWORD = "METECH@3152";
// constexpr char const* ACCESS_POINT_SSID = "Wokwi-GUEST";
// constexpr char const* ACCESS_POINT_PASSWORD = "";

constexpr char const* SECRET = "3e3c9fe7e5d099e1013e8f20c52b46ff0cea526d7bf472fc3195a928284300ce";

// Enums
enum Mode {
    MODE_AUTO,
    MODE_SEMI_AUTO,
    MODE_MANUAL
};

enum State {
    STATE_IDLE,
    STATE_WAIT_SENSOR,
    STATE_WAIT_LOW,
    STATE_STARTING,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_FULL,
    STATE_MANUAL,
    STATE_FAULT
};
