#include "config.h"
#include "system.h"
#include "fsm_controller.h"
#include "display.h"

void setup() {
    pinMode(MODE_BTN, INPUT_PULLUP);
    pinMode(MANUAL_BTN, INPUT_PULLUP);
    pinMode(MOTOR, OUTPUT);
    pinMode(MOTOR_STATUS_LED, OUTPUT);
    pinMode(MODE_LED_RED, OUTPUT);
    pinMode(MODE_LED_GREEN, OUTPUT);
    pinMode(VOLTAGE_SENS, INPUT);
    digitalWrite(MOTOR, LOW);
    digitalWrite(MOTOR_STATUS_LED, LOW);
    digitalWrite(MODE_LED_RED, LOW);
    digitalWrite(MODE_LED_GREEN, HIGH);

    uint32_t now = millis();
    sys.mode = MODE_AUTO;
    sys.state = STATE_IDLE;
    sys.start_th = MOTOR_START_THRESHOLD;
    sys.stop_th  = MOTOR_STOP_THRESHOLD;
    sys.level = 0;
    sys.fault = false;
    sys.lastRetryTime = now;
    sys.dryRunLockUntil = now;
    sys.lastDryCheckTime = now;
    sys.lastDryCheckLevel = sys.level;
    sys.dryRunRetries = 0;
    sys.lastLevelUpdate = now;

    initDisplay();
    welcomeScreen();
    delay(3000);

    sysMutex = xSemaphoreCreateMutex();
    // logQueue = xQueueCreate(10, sizeof(LogMsg));

    xTaskCreatePinnedToCore(control_task, "control", 4096, NULL, 3, NULL, 1);
}

void loop() {

}


