#pragma once

#include <Arduino.h>
#include <SD.h>


// Write
void appendCSV(const char* data);

// Read last N lines into buffer
// Returns number of bytes written
size_t readLastLines(char* outBuf, size_t bufSize, int maxLines);

// Stream full file (for web server)
// bool streamCSV(Stream& out);