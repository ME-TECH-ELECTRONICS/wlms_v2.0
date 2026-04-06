#include "system.h"

// ✅ Actual memory allocation here (ONLY ONCE)
SystemState sys;
SensorPacket pkt;
SemaphoreHandle_t sysMutex;
SemaphoreHandle_t spiMutex;
QueueHandle_t logQueue;
volatile bool otaReady = false;