#include "logger.h"


static SDClass* sd = nullptr;
static MyDS3231* rtc = nullptr;

// ---------- Helper ----------
void getCurrentLogFile(char* filename, size_t len, RTCDateTime dt) {
    snprintf(filename, len, "/log_%04d_%02d.csv", 2000 + dt.year, dt.month);
}

// ---------- Init ----------
void initLogger(SDClass* sdRef, MyDS3231* rtcRef) {
    sd = sdRef;
    rtc = rtcRef;
}

// ---------- Append ----------
void appendCSV(const char* data) {
    if (!sd) return;

    RTCDateTime dt = rtc->getDateTime();

    char filename[32];
    getCurrentLogFile(filename, sizeof(filename), dt);

    File f = sd->open(filename, FILE_WRITE);
    if (!f) return;

    if (f.size() == 0) {
        f.println("date,time,level,voltage,motor");
    }

    f.println(data);
    f.close();
}

// ---------- Read last N lines (NO String) ----------
size_t readLastLines(char* outBuf, size_t bufSize, int maxLines) {
    if (!sd || !outBuf || bufSize == 0) return 0;

    RTCDateTime dt = rtc->getDateTime();

    char filename[32];
    getCurrentLogFile(filename, sizeof(filename), dt);

    File f = sd->open(filename);
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
//     if (!sd) return false;

//     RTCDateTime dt = rtc->getDateTime();

//     char filename[32];
//     getCurrentLogFile(filename, sizeof(filename), dt);

//     File f = sd->open(filename);
//     if (!f) return false;

//     uint8_t buffer[128];

//     while (f.available()) {
//         size_t n = f.read(buffer, sizeof(buffer));
//         out.write(buffer, n);
//     }

//     f.close();
//     return true;
// }