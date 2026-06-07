#include "system.h"

SystemState sys;
SemaphoreHandle_t sysMutex;
SemaphoreHandle_t spiMutex;
SemaphoreHandle_t i2cMutex;
QueueHandle_t logQueue;
TaskHandle_t displayHandle;
QueueHandle_t loraQueue;