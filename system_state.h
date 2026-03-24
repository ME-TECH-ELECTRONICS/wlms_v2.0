#pragma once
#include <stdint.h>

enum Mode {
    MODE_AUTO,
    MODE_SEMI_AUTO,
    MODE_MANUAL
};

enum MotorState {
    MOTOR_OFF,
    MOTOR_ON
};

struct SystemState {
    float water_level;
    float voltage;
    Mode mode;
    MotorState motor;
    bool is_daytime;
    uint32_t last_lora_update;
};

extern SystemState g_state;