#include <WiFi.h>
#include "config.h"
#include "system.h"
#include "fsm_controller.h"
#include "display.h"
#include "web_dash.h"
#include "lora_rx.h"

time_t RTCnow;
struct tm timeinfo;

#include <WiFi.h>

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      WiFi.reconnect();
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      break;

    default:
      break;
  }
}

void printTaskStats() {
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    TaskStatus_t *taskArray = (TaskStatus_t *)malloc(taskCount * sizeof(TaskStatus_t));

    if (!taskArray) return;

    uint32_t totalRuntime;
    taskCount = uxTaskGetSystemState(taskArray, taskCount, &totalRuntime);

    Serial.println("\n===== Task Stats =====");

    for (int i = 0; i < taskCount; i++) {
        Serial.printf("Name: %s | Core: %d | Prio: %d | State: %d | Stack Left: %d words (%d bytes)\n",
            taskArray[i].pcTaskName,
            taskArray[i].xCoreID,
            taskArray[i].uxCurrentPriority,
            taskArray[i].eCurrentState,
            taskArray[i].usStackHighWaterMark,
            taskArray[i].usStackHighWaterMark * 4
        );
    }
    Serial.println("\n==========================");
    free(taskArray);
}

void monitorTask(void* pv) {
  while (1) {
    printTaskStats();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
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
  sys.stop_th = MOTOR_STOP_THRESHOLD;
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
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ACCESS_POINT_SSID, ACCESS_POINT_PASSWORD);
  Serial.print("Connecting to WiFi");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (true) {
    time(&RTCnow);
    localtime_r(&RTCnow, &timeinfo);

    if (timeinfo.tm_year > (2020 - 1900)) {
      break;  // valid time
    }

    delay(500);
    Serial.print(".");
  }
  web_dash_init();

  Serial.println("Web Dashboard is ready!");
  Serial.println("Open: http://" + WiFi.localIP().toString());
  sysMutex = xSemaphoreCreateMutex();
  spiMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  // logQueue = xQueueCreate(10, sizeof(LogMsg));
  xTaskCreatePinnedToCore(control_task, "control", 4096, NULL, 3, NULL, 1);
  // xTaskCreatePinnedToCore(loraTask, "LoRa", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(displayTask, "Display", 1700, NULL, 3, &displayHandle, 1);
  xTaskCreatePinnedToCore(monitorTask, "Monitor", 2048, NULL, 3, NULL, 1);
}

void loop() {
  
}
