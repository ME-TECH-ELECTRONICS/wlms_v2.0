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

void control_task(void *pv) {
    while (1) {
        uint32_t now = millis();
        bool logNeeded = false;
        const char* logMsg = NULL;
        xSemaphoreTake(sysMutex, portMAX_DELAY);
        
        if (sys.state == STATE_FAULT) {
            xSemaphoreGive(sysMutex);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // Sensor timeout
        if ((now - sys.lastLevelUpdate) > SENSOR_TIMEOUT_MS) {
            if (sys.motor) motor_off();
            sys.motor = false;
            sys.state = STATE_FAULT;
            sys.fault = true;
            logNeeded = true;
            logMsg = "FAULT: SENSOR TIMEOUT";
        }

        // Motor runtime limit
        if (sys.motor && (now - sys.motorStartTime > MAX_MOTOR_RUNTIME_MS)) {
            if (sys.motor) motor_off();
            sys.motor = false;
            sys.state = STATE_FAULT;
            sys.fault = true;
            logNeeded = true;
            logMsg ="FAULT: MOTOR TIMEOUT";
        }

        switch (sys.state) {

            case STATE_IDLE:
                sys.state = STATE_WAIT_LOW;
                break;
    
            case STATE_WAIT_LOW:
                if (sys.mode == MODE_AUTO && sys.level <= sys.start_th - 2) {
                    sys.state = STATE_STARTING;
                    logNeeded = true;
                    logMsg ="LOW LEVEL DETECTED";
                }
                break;
    
            case STATE_STARTING:
                if (sys.voltage >= VOLTAGE_MIN && sys.isDay) {
                
                    if (!sys.motor) {
                        motor_on();
                        sys.motor = true;
    
                        sys.motorStartTime = now;
    
                        // Initialize dry run tracking
                        sys.lastDryCheckTime = now;
                        sys.lastDryCheckLevel = sys.level;
                        sys.dryRunRetries = 0;
    
                        logNeeded = true;
                        logMsg ="MOTOR ON";
                    }
    
                    sys.state = STATE_RUNNING;
    
                } else {
                    sys.state = STATE_BLOCKED;
                    logNeeded = true;
                    logMsg ="BLOCKED (LOW VOLT/NIGHT)";
                }
                break;
    
            case STATE_RUNNING:
    
                // -------- NORMAL STOP CONDITIONS --------
                if (sys.voltage <= VOLTAGE_FAIL) {
                    if (sys.motor) motor_off();
                    sys.motor = false;
                    sys.state = STATE_BLOCKED;
                    logNeeded = true;
                    logMsg ="VOLTAGE DROP - MOTOR OFF";
                    break;
                }
    
                if (sys.level >= sys.stop_th) {
                    if (sys.motor) motor_off();
                    sys.motor = false;
                    sys.state = STATE_FULL;
                    logNeeded = true;
                    logMsg ="TANK FULL - MOTOR OFF";
                    break;
                }
    
                if (now - sys.motorStartTime < 10000) break;
                // -------- DRY RUN DETECTION --------
                if (now - sys.lastDryCheckTime >= DRYRUN_CHECK_INTERVAL_MS) {
                
                    int levelDiff = (int)sys.level - (int)sys.lastDryCheckLevel;
    
                    if (levelDiff < DRYRUN_MIN_INCREASE) {
                    
                        sys.dryRunRetries++;
                        sys.dryRunLockUntil = now + 30000; // 30 sec lock
                        logNeeded = true;
                        logMsg ="DRY RUN DETECTED";
    
                        if (sys.motor) motor_off();
                        sys.motor = false;
    
                        if (sys.dryRunRetries <= DRYRUN_MAX_RETRIES) {
                            sys.state = STATE_BLOCKED;
                            logNeeded = true;
                            logMsg ="RETRY AFTER DRY RUN";
                        } else {
                            sys.state = STATE_FAULT;
                            sys.fault = true;
                            logNeeded = true;
                            logMsg ="FAULT: DRY RUN";
                        }
    
                    } else {
                        // Water increasing normally
                        sys.dryRunRetries = 0;
                    }
    
                    // Update reference point
                    sys.lastDryCheckLevel = sys.level;
                    sys.lastDryCheckTime = now;
                }
    
                break;
    
            case STATE_BLOCKED:
                if (now < sys.dryRunLockUntil) break;
                if (now - sys.lastRetryTime > 5000) {
                    if (sys.voltage >= VOLTAGE_MIN && sys.isDay) {
                        sys.state = STATE_STARTING;
                        logNeeded = true;
                        logMsg ="RETRY MOTOR";
                        sys.lastRetryTime = now;
                    }
                }
                break;
    
            case STATE_FULL:
                if (sys.motor) motor_off();
                sys.motor = false;
                sys.state = STATE_WAIT_LOW;
                break;
    
            case STATE_MANUAL:
                // Optional: still enforce safety
                if (sys.voltage <= VOLTAGE_FAIL) {
                    if (sys.motor) motor_off();
                    sys.motor = false;
                    sys.state = STATE_FAULT;
                    logNeeded = true;
                    logMsg ="FAULT: MANUAL VOLTAGE DROP";
                }
                break;
    
            case STATE_FAULT:
                break;
        }
        xSemaphoreGive(sysMutex);
        if(logNeeded) log_event(logMsg);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}