#include "voltage_sense.h"

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
                  taskArray[i].usStackHighWaterMark * 4);
  }
  Serial.print("\n==========================");
  free(taskArray);
}


void monitorTask(void *pv) {
  while (1) {
    printTaskStats();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}


void setup() {
  Serial.begin(115200);
  initADC();

  xTaskCreatePinnedToCore(readADC, "ADC Task", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(readVoltageTask, "RMS Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(monitorTask, "Monitor", 2048, NULL, 1, NULL, 1);
}

void loop() {
}