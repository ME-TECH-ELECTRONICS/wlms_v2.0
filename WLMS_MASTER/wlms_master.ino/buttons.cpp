#include "esp32-hal-touch-ng.h"
#include "buttons.h"
#include "config.h"
#include "system.h"

void buttonTask(void *pv) {
  bool modeTouched = false;
  bool manualTouched = false;
  Serial.print("Button event handler started");
  while (1) {
    bool modeNow = touchRead(T6) < TOUCH_THRESHOLD;
    bool manualNow = touchRead(T7) < TOUCH_THRESHOLD;
    // Serial.print("mode=");
    // Serial.print(touchRead(T6));
    // Serial.print(" manual=");
    // Serial.println(touchRead(T7));
    // MODE TOUCH
    if (modeNow && !modeTouched) {
      modeTouched = true;
      xSemaphoreTake(sysMutex, portMAX_DELAY);
      if (sys.mode == MODE_AUTO) {
        sys.mode = MODE_MANUAL;
        Serial.println("MODE -> MANUAL");
      } else {
        sys.mode = MODE_AUTO;
        Serial.println("MODE -> AUTO");
      }
      xSemaphoreGive(sysMutex);
    }

    if (!modeNow)
      modeTouched = false;

    // MANUAL TOUCH
    if (manualNow && !manualTouched) {
      manualTouched = true;
      xSemaphoreTake(sysMutex, portMAX_DELAY);
      if (sys.mode == MODE_MANUAL) {
        if (!sys.motor) {
          if (sys.voltage >= VOLTAGE_MIN) {
            sys.motor = true;
            sys.state = STATE_RUNNING;
            sys.motorStartTime = millis();
            sys.lastDryCheckTime = millis();
            sys.lastDryCheckLevel = sys.level;
            sys.dryRunRetries = 0;

            Serial.println("MANUAL MOTOR ON");

          } else {
            Serial.println("LOW VOLTAGE - START BLOCKED");
          }
        } else {
          sys.motor = false;
          sys.state = STATE_WAIT_LOW;

          Serial.println("MANUAL MOTOR OFF");
        }
      }
      xSemaphoreGive(sysMutex);
    }
    if (!manualNow)
      manualTouched = false;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}