#include <soc/gpio_struct.h>

#include "fsm_controller.h"
#include "config.h"
#include "system.h"

inline void motor_on() {
  GPIO.out_w1ts = (1 << MOTOR);
  GPIO.out1_w1ts.val = (1 << (MOTOR_STATUS_LED - 32));
}

inline void motor_off() {
  GPIO.out_w1tc = (1 << MOTOR);
  GPIO.out1_w1tc.val = (1 << (MOTOR_STATUS_LED - 32));
}

inline void log_event(const char* msg) {
  __asm__ __volatile__("nop");
}
int prevLvl = 0;
void control_task(void* pv) {
  static uint32_t faultTime = 0;
  while (1) {
    uint32_t now = millis();

    // -------- STEP 1: COPY SHARED STATE --------
    SystemState local;
    xSemaphoreTake(sysMutex, portMAX_DELAY);
    local = sys;
    xSemaphoreGive(sysMutex);

    // -------- LOCAL FLAGS --------
    bool logNeeded = false;
    const char* logMsg = "";

    bool turnMotorOn = false;
    bool turnMotorOff = false;

    // -------- FSM LOGIC (NO MUTEX) --------

    if (local.state == STATE_FAULT) {
      if (faultTime == 0) faultTime = now;

      // wait 10 seconds before retry
      if ((now - faultTime) > 10000) {

        if ((now - local.lastLevelUpdate) < SENSOR_TIMEOUT_MS && local.voltage >= VOLTAGE_MIN) {

          local.state = STATE_WAIT_LOW;
          local.fault = false;
          faultTime = 0;

          log_event("AUTO RECOVERY");
        }
      }

      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // Sensor timeout
    if ((now - local.lastLevelUpdate) > SENSOR_TIMEOUT_MS) {
      turnMotorOff = true;
      local.motor = false;
      local.state = STATE_FAULT;
      local.fault = true;
      logNeeded = true;
      logMsg = "FAULT: SENSOR TIMEOUT";
    }

    // Motor runtime limit
    if (local.motor && (now - local.motorStartTime > MAX_MOTOR_RUNTIME_MS)) {
      turnMotorOff = true;
      local.motor = false;
      local.state = STATE_FAULT;
      local.fault = true;
      logNeeded = true;
      logMsg = "FAULT: MOTOR TIMEOUT";
    }
    bool sensorValid = (millis() - local.lastLevelUpdate) < SENSOR_TIMEOUT_MS;
    bool sensorOK = sensorValid && (local.level > 0 && local.level <= 100);
    switch (local.state) {

      case STATE_IDLE:
        local.motor = false;
        turnMotorOff = true;
        local.state = STATE_WAIT_SENSOR;
        break;

      case STATE_WAIT_SENSOR:

        if (sensorValid && sensorOK) {
          local.state = STATE_WAIT_LOW;
        }
        break;

      case STATE_WAIT_LOW:
        if (local.mode == MODE_AUTO && local.level <= local.start_th) {
          local.state = STATE_STARTING;
          logNeeded = true;
          logMsg = "LOW LEVEL DETECTED";
        }
        break;

      case STATE_STARTING:
        if (local.voltage >= VOLTAGE_MIN && local.isDay) {

          if (!local.motor) {
            turnMotorOn = true;
            local.motor = true;

            local.motorStartTime = now;
            local.lastDryCheckTime = now;
            local.lastDryCheckLevel = local.level;
            local.dryRunRetries = 0;

            logNeeded = true;
            logMsg = "MOTOR ON";
          }

          local.state = STATE_RUNNING;

        } else {
          local.state = STATE_BLOCKED;
          logNeeded = true;
          logMsg = "BLOCKED (LOW VOLT/NIGHT)";
        }
        break;

      case STATE_RUNNING:

        // Normal stop conditions
        if (local.voltage <= VOLTAGE_FAIL) {
          turnMotorOff = true;
          local.motor = false;
          local.state = STATE_BLOCKED;
          logNeeded = true;
          logMsg = "VOLTAGE DROP - MOTOR OFF";
          break;
        }

        if (local.level >= local.stop_th) {
          turnMotorOff = true;
          local.motor = false;
          local.state = STATE_FULL;
          logNeeded = true;
          logMsg = "TANK FULL - MOTOR OFF";
          break;
        }

        if (now - local.motorStartTime < 10000) break;

        // Dry run detection
        if (now - local.lastDryCheckTime >= DRYRUN_CHECK_INTERVAL_MS) {

          int levelDiff = (int)local.level - (int)local.lastDryCheckLevel;

          if (levelDiff < DRYRUN_MIN_INCREASE) {

            local.dryRunRetries++;
            local.dryRunLockUntil = now + 30000;

            turnMotorOff = true;
            local.motor = false;

            if (local.dryRunRetries <= DRYRUN_MAX_RETRIES) {
              local.state = STATE_BLOCKED;
              logMsg = "RETRY AFTER DRY RUN";
            } else {
              local.state = STATE_FAULT;
              local.fault = true;
              logMsg = "FAULT: DRY RUN";
            }

            logNeeded = true;

          } else {
            local.dryRunRetries = 0;
          }

          local.lastDryCheckLevel = local.level;
          local.lastDryCheckTime = now;
        }

        break;

      case STATE_BLOCKED:
        if (now < local.dryRunLockUntil) break;

        if (now - local.lastRetryTime > 5000) {
          if (local.voltage >= VOLTAGE_MIN && local.isDay) {
            local.state = STATE_STARTING;
            local.lastRetryTime = now;
            logNeeded = true;
            logMsg = "RETRY MOTOR";
          }
        }
        break;

      case STATE_FULL:
        turnMotorOff = true;
        local.motor = false;
        local.state = STATE_WAIT_LOW;
        break;

      case STATE_MANUAL:
        if (local.voltage <= VOLTAGE_FAIL) {
          turnMotorOff = true;
          local.motor = false;
          local.state = STATE_FAULT;
          logNeeded = true;
          logMsg = "FAULT: MANUAL VOLTAGE DROP";
        }
        break;

      case STATE_FAULT:
        break;
    }

    // -------- STEP 3: WRITE BACK SHARED STATE --------
    xSemaphoreTake(sysMutex, portMAX_DELAY);
    sys = local;
    xSemaphoreGive(sysMutex);

    // -------- STEP 4: HARDWARE CONTROL (NO MUTEX) --------
    if (turnMotorOn) motor_on();
    if (turnMotorOff) motor_off();

    // -------- STEP 5: LOGGING --------
    if (logNeeded && logMsg) log_event(logMsg);
    if (local.level != prevLvl) {
      Serial.printf("L:%d V:%d Vol:%d Motor:%d WiFi:%d Mains:%d Msg: %s\n", local.level, local.voltage, local.level * 10, local.motor, local.isWifiConnected, local.isMainsCut, logMsg);
      prevLvl = local.level;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}