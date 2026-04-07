#include "system.h"

SystemState sys;
SensorPacket pkt;
SemaphoreHandle_t sysMutex;
SemaphoreHandle_t spiMutex;
SemaphoreHandle_t i2cMutex;
QueueHandle_t logQueue;
TaskHandle_t displayHandle;