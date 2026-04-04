#pragma once

#include <Arduino.h>
#include <SD.h>
#include "rtc.h"

// Init
void initLogger(SDClass* sdRef, MyDS3231* rtcRef);

// Write
void appendCSV(const char* data);

// Read last N lines into buffer
// Returns number of bytes written
size_t readLastLines(char* outBuf, size_t bufSize, int maxLines);

// Stream full file (for web server)
// bool streamCSV(Stream& out);