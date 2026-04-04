#include "rtc.h"
#include <Wire.h>
// #include <WiFi.h>
// #include <time.h>

#define DS3231_ADDR 0x68

// static SemaphoreHandle_t rtcMutex;

// ---------- BCD Helpers ----------
static inline uint8_t bcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

static inline uint8_t decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

// ---------- Init ----------
void MyDS3231::begin()
{
    Wire.begin();
    // rtcMutex = xSemaphoreCreateMutex();
}

// ---------- OSF Check ----------
bool MyDS3231::isTimeValid()
{
    // xSemaphoreTake(rtcMutex, portMAX_DELAY);

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDR, 1);
    uint8_t status = Wire.read();

    // xSemaphoreGive(rtcMutex);

    return !(status & 0x80);
}

// ---------- Clear OSF ----------
void MyDS3231::clearOSF()
{
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDR, 1);
    uint8_t status = Wire.read();

    status &= ~0x80;

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.write(status);
    Wire.endTransmission();
}

// ---------- Read Time ----------
RTCDateTime MyDS3231::getDateTime()
{
    RTCDateTime dt;

    // xSemaphoreTake(rtcMutex, portMAX_DELAY);

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDR, 7);

    dt.second = bcdToDec(Wire.read());
    dt.minute = bcdToDec(Wire.read());
    dt.hour = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); // skip DOW
    dt.day = bcdToDec(Wire.read());
    dt.month = bcdToDec(Wire.read() & 0x1F);
    dt.year = bcdToDec(Wire.read());

    // xSemaphoreGive(rtcMutex);

    return dt;
}

// ---------- Format ----------
void MyDS3231::formatDateTime(char *buf)
{
    RTCDateTime dt = getDateTime();
    buf[0] = '0' + dt.day / 10;
    buf[1] = '0' + dt.day % 10;
    buf[2] = '/';

    buf[3] = '0' + dt.month / 10;
    buf[4] = '0' + dt.month % 10;
    buf[5] = '/';

    buf[6] = '0' + dt.year / 10;
    buf[7] = '0' + dt.year % 10;
    buf[8] = ' ';

    buf[9] = '0' + dt.hour / 10;
    buf[10] = '0' + dt.hour % 10;
    buf[11] = ':';

    buf[12] = '0' + dt.minute / 10;
    buf[13] = '0' + dt.minute % 10;

    buf[14] = '\0';
}

// ---------- Set RTC from NTP ----------
// void MyDS3231::setDateTimeFromNTP() {

//     configTime(0, 0, "pool.ntp.org", "time.nist.gov");

//     struct tm timeinfo;
//     int retry = 0;

//     while (!getLocalTime(&timeinfo) && retry < 10) {
//         // vTaskDelay(pdMS_TO_TICKS(500));
//         retry++;
//     }

//     if (retry == 10) return; // failed

//     // xSemaphoreTake(rtcMutex, portMAX_DELAY);

//     Wire.beginTransmission(DS3231_ADDR);
//     Wire.write(0x00);

//     Wire.write(decToBcd(timeinfo.tm_sec));
//     Wire.write(decToBcd(timeinfo.tm_min));
//     Wire.write(decToBcd(timeinfo.tm_hour));
//     Wire.write(decToBcd(timeinfo.tm_wday));
//     Wire.write(decToBcd(timeinfo.tm_mday));
//     Wire.write(decToBcd(timeinfo.tm_mon + 1));
//     Wire.write(decToBcd(timeinfo.tm_year % 100));

//     Wire.endTransmission();

//     clearOSF();

//     // xSemaphoreGive(rtcMutex);
// }

// ---------- Auto Sync ----------
// bool MyDS3231::syncIfNeeded() {

//     if (isTimeValid()) return true;

//     // Must have WiFi connected before calling this
//     setDateTimeFromNTP();

//     return isTimeValid();
// }

void MyDS3231::setTimeFromCompile()
{
    const char *date = __DATE__; // "Apr 04 2026"
    const char *time = __TIME__; // "11:30:25"

    char monthStr[4];
    int day, year;
    int hour, minute, second;

    // Extract month
    monthStr[0] = date[0];
    monthStr[1] = date[1];
    monthStr[2] = date[2];
    monthStr[3] = '\0';

    // Extract day (handles space padding)
    day = (date[4] == ' ') ? (date[5] - '0') : ((date[4] - '0') * 10 + (date[5] - '0'));

    // Extract year
    year = (date[7] - '0') * 1000 +
           (date[8] - '0') * 100 +
           (date[9] - '0') * 10 +
           (date[10] - '0');

    // Extract time
    hour = (time[0] - '0') * 10 + (time[1] - '0');
    minute = (time[3] - '0') * 10 + (time[4] - '0');
    second = (time[6] - '0') * 10 + (time[7] - '0');

    // Convert month string → number
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *p = strstr(months, monthStr);
    uint8_t month = p ? ((p - months) / 3 + 1) : 1;

    // Convert to 2-digit year
    uint8_t yr = year % 100;

    // Write to DS3231
    Wire.beginTransmission(0x68);
    Wire.write(0x00);

    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));
    Wire.write(1); // Day of week (not important)
    Wire.write(decToBcd(day));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(yr));

    Wire.endTransmission();

    // Clear OSF (important)
    Wire.beginTransmission(0x68);
    Wire.write(0x0F);
    Wire.write(0x00);
    Wire.endTransmission();
}