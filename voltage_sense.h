#pragma once
#include <Arduino.h>

void initADC();
void readADC(void *pv);
void readVoltageTask(void *pv); 