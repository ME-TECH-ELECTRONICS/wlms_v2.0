#include "system.h"

// ✅ Actual memory allocation here (ONLY ONCE)
SystemState sys;
SemaphoreHandle_t sysMutex;
QueueHandle_t logQueue;