#include "fsm_controller.h"

#include <soc/gpio_struct.h>
#include "config.h"
#include "system.h"

inline void motor_on() {
  digitalWrite(MOTOR, HIGH);
  digitalWrite(MOTOR_STATUS_LED, HIGH);
}

inline void motor_off() {
  digitalWrite(MOTOR, LOW);
  digitalWrite(MOTOR_STATUS_LED, LOW);
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
    Settings cfg;
    xSemaphoreTake(sysMutex, portMAX_DELAY);
    local = sys;
    cfg = settings;
    xSemaphoreGive(sysMutex);

    // -------- LOCAL FLAGS --------
    bool logNeeded = false;
    const char* logMsg = "";

    // -------- FSM LOGIC (NO MUTEX) --------

    bool sensorValid = (millis() - local.lastLevelUpdate) < cfg.sensorTimeoutMs;
    bool sensorOK = sensorValid && (local.level >= 0 && local.level <= 100);
    // Sensor timeout
    if (!sensorOK) {
      local.motor = false;
      local.state = STATE_FAULT;
      local.fault = true;
      logNeeded = true;
      logMsg = "FAULT: SENSOR TIMEOUT";
    }

    // Motor runtime limit
    if (local.motor && (now - local.motorStartTime > cfg.maxMotorRuntimeMs)) {
      local.motor = false;
      local.state = STATE_FAULT;
      local.fault = true;
      logNeeded = true;
      logMsg = "FAULT: MOTOR TIMEOUT";
    }

    switch (local.state) {

      case STATE_IDLE:
        local.motor = false;
        local.state = STATE_WAIT_SENSOR;
        break;

      case STATE_WAIT_SENSOR:
        if (sensorValid && sensorOK) {
          local.state = STATE_WAIT_LOW;
        }
        break;

      case STATE_WAIT_LOW:
        if (local.mode == MODE_AUTO && local.level <= local.start_th) {
          Serial.printf(
            "WAIT_LOW -> STARTING  level=%d start_th=%d\n",
            local.level,
            local.start_th);
          local.state = STATE_STARTING;
          logNeeded = true;
          logMsg = "LOW LEVEL DETECTED";
        }
        break;

      case STATE_STARTING:
        Serial.printf(
          "ENTER STARTING  motor=%d voltage=%d isDay=%d\n",
          local.motor,
          local.voltage,
          local.isDay);

        if (local.voltage >= cfg.voltageMin && local.isDay) {
          if (!local.motor) {
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
        if (local.voltage <= cfg.voltageFail) {
          local.motor = false;
          local.state = STATE_BLOCKED;
          logNeeded = true;
          logMsg = "VOLTAGE DROP - MOTOR OFF";
          break;
        }

        if (local.level >= local.stop_th) {
          local.motor = false;
          local.state = STATE_FULL;
          logNeeded = true;
          logMsg = "TANK FULL - MOTOR OFF";
          break;
        }

        if (now - local.motorStartTime < cfg.dryRunStartupDelayMs) break;
        // Dry run detection
        if (now - local.lastDryCheckTime >= cfg.dryRunIntervalMs) {
          int levelDiff = (int)local.level - (int)local.lastDryCheckLevel;
          if (levelDiff < cfg.dryRunMinIncrease) {
            local.dryRunRetries++;
            local.dryRunLockUntil = now + cfg.dryRunLockMs;
            local.motor = false;

            if (local.dryRunRetries <= cfg.dryRunMaxRetries) {
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
        if (now - local.lastRetryTime > cfg.retryDelayMs) {
          if (local.voltage >= cfg.voltageMin && local.isDay) {
            local.state = STATE_STARTING;
            local.lastRetryTime = now;
            logNeeded = true;
            logMsg = "RETRY MOTOR";
          }
        }
        break;

      case STATE_FULL:
        local.motor = false;
        local.state = STATE_WAIT_LOW;
        break;

      case STATE_MANUAL:
        if (local.voltage <= cfg.voltageFail) {
          local.motor = false;
          local.state = STATE_FAULT;
          logNeeded = true;
          logMsg = "FAULT: MANUAL VOLTAGE DROP";
        }
        if (local.level >= local.stop_th) {
          local.motor = false;
          local.state = STATE_FULL;
          logNeeded = true;
          logMsg = "TANK FULL - MOTOR OFF";
          break;
        }
        if (now - local.motorStartTime > cfg.dryRunStartupDelayMs) {

          if (now - local.lastDryCheckTime >= cfg.dryRunIntervalMs) {

            int levelDiff =
              (int)local.level - (int)local.lastDryCheckLevel;

            if (levelDiff < cfg.dryRunMinIncrease) {

              local.motor = false;
              local.state = STATE_FAULT;
              local.fault = true;

              logNeeded = true;
              logMsg = "FAULT: DRY RUN";
            }

            local.lastDryCheckLevel = local.level;
            local.lastDryCheckTime = now;
          }
        }

        break;

      case STATE_FAULT:
        if (faultTime == 0) faultTime = now;

        // wait 10 seconds before retry
        if ((now - faultTime) > cfg.faultRecoveryTimeMs) {
          if ((now - local.lastLevelUpdate) < cfg.sensorTimeoutMs && local.voltage >= cfg.voltageMin) {
            local.state = STATE_WAIT_LOW;
            local.fault = false;
            local.motor = false;
            faultTime = 0;
            log_event("AUTO RECOVERY");
          }
        }
        break;
    }

    // -------- STEP 3: WRITE BACK SHARED STATE --------
    xSemaphoreTake(sysMutex, portMAX_DELAY);

    sys.motor = local.motor;
    sys.state = local.state;
    sys.motorStartTime = local.motorStartTime;
    sys.fault = local.fault;
    sys.dryRunLockUntil = local.dryRunLockUntil;
    sys.lastDryCheckTime = local.lastDryCheckTime;
    sys.lastDryCheckLevel = local.lastDryCheckLevel;
    sys.dryRunRetries = local.dryRunRetries;
    sys.lastRetryTime = local.lastRetryTime;

    xSemaphoreGive(sysMutex);

    // -------- STEP 4: HARDWARE CONTROL (NO MUTEX) --------
    if (local.motor) {
      motor_on();
    } else {
      motor_off();
    }

    // -------- STEP 5: LOGGING --------
    if (logNeeded && logMsg) log_event(logMsg);
    if (local.level != prevLvl) {
      Serial.printf("L:%d V:%d Vol:%d Motor:%d WiFi:%d Mains:%d Msg: %s\n", local.level, local.voltage, local.level * 10, local.motor, local.isWifiConnected, local.isMainsCut, logMsg);
      prevLvl = local.level;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}