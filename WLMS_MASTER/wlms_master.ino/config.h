#include <stdint.h>
#pragma once

// Pins
constexpr uint8_t MOTOR = 15;
constexpr uint8_t MOTOR_STATUS_LED = 33;
constexpr uint8_t MODE_LED_RED = 25;
constexpr uint8_t MODE_LED_GREEN = 26;
constexpr uint8_t MODE_BTN = 27;
constexpr uint8_t MANUAL_BTN = 14;
constexpr uint8_t VOLTAGE_SENS = 13;
constexpr uint8_t LORA_RST = 16;
constexpr uint8_t LORA_D0 = 4;
constexpr uint8_t LORA_CHIP_SELECT = 5;
constexpr uint8_t SDCARD_CHIP_SELECT = 17;

// Thresholds
constexpr uint8_t MOTOR_START_THRESHOLD = 10;
constexpr uint8_t MOTOR_STOP_THRESHOLD = 100;

constexpr uint8_t DAY_START_HOUR = 6;
constexpr uint8_t DAY_END_HOUR = 18;

constexpr uint8_t VOLTAGE_MIN = 220;
constexpr uint8_t VOLTAGE_MAX = 250;
constexpr uint8_t VOLTAGE_FAIL = 110;

// Timing
constexpr uint32_t DRYRUN_CHECK_INTERVAL_MS = 15000;
constexpr uint8_t DRYRUN_MIN_INCREASE = 2;
constexpr uint8_t DRYRUN_MAX_RETRIES = 2;
constexpr uint32_t SENSOR_TIMEOUT_MS = 30000UL;
constexpr uint32_t MAX_MOTOR_RUNTIME_MS = (20UL * 60UL * 1000UL);

constexpr char const* ACCESS_POINT_SSID = "METECH";
constexpr char const* ACCESS_POINT_PASSWORD = "METECH@3152";
constexpr char const* WEB_DASH_USERNAME = "METECH";
constexpr char const* WEB_DASH_PASSWORD = "METECH3152";
constexpr char const* WEB_DASH_SECRET = "METECH@3152";

// Enums
enum Mode {
    MODE_AUTO,
    MODE_SEMI_AUTO,
    MODE_MANUAL
};

enum State {
    STATE_IDLE,
    STATE_WAIT_LOW,
    STATE_STARTING,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_FULL,
    STATE_MANUAL,
    STATE_FAULT
};