#include "HardwareSerial.h"
#include "logger.h"
#include <SD.h>

// ---------- Helper ----------
void getCurrentLogFile(char* filename, size_t len, uint8_t year, uint8_t month) {
  snprintf(filename, len, "/log%02d_%02d.csv", month, year);
}

// ---------- Append ----------
void appendCSV(const char* data) {

  char filename[32];
  getCurrentLogFile(filename, sizeof(filename), 26, 4);

  File f = SD.open(filename, FILE_WRITE);
  Serial.println(filename);
  if (!f) {
    Serial.println("no file");
    return;
  };
  Serial.println(data);
  if (f.size() == 0) {
    f.println("date,time,level,voltage,motor");
  }

  f.println(data);
  f.close();
}

// ---------- Read last N lines (NO String) ----------
size_t readLastLines(char* outBuf, size_t bufSize, int maxLines) {
  if (!outBuf || bufSize == 0) return 0;

  char filename[32];
  getCurrentLogFile(filename, sizeof(filename), 26, 4);

  File f = SD.open(filename);
  if (!f) return 0;

  int lines = 0;
  int pos = f.size() - 1;

  size_t idx = bufSize - 1;  // fill backwards
  outBuf[idx] = '\0';

  while (pos >= 0 && lines < maxLines && idx > 0) {
    f.seek(pos);
    char c = f.read();

    if (c == '\n') {
      lines++;
      if (lines > maxLines) break;
    }

    outBuf[--idx] = c;
    pos--;
  }

  f.close();

  // shift to start of buffer
  size_t len = bufSize - 1 - idx;
  memmove(outBuf, &outBuf[idx], len);
  outBuf[len] = '\0';

  return len;
}

// ---------- Stream full CSV ----------
// bool streamCSV(Stream& out) {

//     char filename[32];
//     getCurrentLogFile(filename, sizeof(filename), dt);

//     File f = SD.open(filename);
//     if (!f) return false;

//     uint8_t buffer[128];

//     while (f.available()) {
//         size_t n = f.read(buffer, sizeof(buffer));
//         out.write(buffer, n);
//     }

//     f.close();
//     return true;
// }