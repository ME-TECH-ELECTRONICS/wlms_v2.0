#include "rtc.h"
#include <Wire.h>

#define DS3231_ADDR 0x68

// ---------- BCD Helpers ----------
static inline uint8_t bcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static inline uint8_t decToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

// ---------- Init ----------
void MyDS3231::begin() {
    Wire.begin();
}

// ---------- OSF Check ----------
bool MyDS3231::isTimeValid() {
    

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);

    if (Wire.endTransmission() != 0) {
        
        return false;
    }

    Wire.requestFrom(DS3231_ADDR, 1);
    if (Wire.available() < 1) {
        
        return false;
    }

    uint8_t status = Wire.read();

    

    return !(status & 0x80);
}

// ---------- Clear OSF ----------
void MyDS3231::clearOSF() {
    

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDR, 1);
    if (Wire.available() < 1) {
        
        return;
    }

    uint8_t status = Wire.read();
    status &= ~0x80;

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.write(status);
    Wire.endTransmission();

    
}

// ---------- Read Time ----------
RTCDateTime MyDS3231::getDateTime() {
    RTCDateTime dt = {0};

    

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);

    if (Wire.endTransmission() != 0) {
        
        return dt;
    }

    Wire.requestFrom(DS3231_ADDR, 7);
    if (Wire.available() < 7) {
        
        return dt;
    }

    dt.second = bcdToDec(Wire.read());
    dt.minute = bcdToDec(Wire.read());
    dt.hour   = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); // skip DOW
    dt.day   = bcdToDec(Wire.read());
    dt.month = bcdToDec(Wire.read() & 0x1F);
    dt.year  = bcdToDec(Wire.read());

    

    return dt;
}

// ---------- Format ----------
void MyDS3231::formatDateTime(const RTCDateTime& dt, char* buf) {
    buf[0] = '0' + dt.day / 10;
    buf[1] = '0' + dt.day % 10;
    buf[2] = '/';

    buf[3] = '0' + dt.month / 10;
    buf[4] = '0' + dt.month % 10;
    buf[5] = '/';

    buf[6] = '0' + dt.year / 10;
    buf[7] = '0' + dt.year % 10;
    buf[8] = ' ';

    buf[9]  = '0' + dt.hour / 10;
    buf[10] = '0' + dt.hour % 10;
    buf[11] = ':';

    buf[12] = '0' + dt.minute / 10;
    buf[13] = '0' + dt.minute % 10;

    buf[14] = '\0';
}

// ---------- Set from Compile Time ----------
void MyDS3231::setTimeFromCompile() {
    const char *date = __DATE__;
    const char *time = __TIME__;

    char monthStr[4];
    int day, year;
    int hour, minute, second;

    monthStr[0] = date[0];
    monthStr[1] = date[1];
    monthStr[2] = date[2];
    monthStr[3] = '\0';

    day = (date[4] == ' ') ? (date[5] - '0') :
          ((date[4] - '0') * 10 + (date[5] - '0'));

    year = (date[7] - '0') * 1000 +
           (date[8] - '0') * 100 +
           (date[9] - '0') * 10 +
           (date[10] - '0');

    hour   = (time[0] - '0') * 10 + (time[1] - '0');
    minute = (time[3] - '0') * 10 + (time[4] - '0');
    second = (time[6] - '0') * 10 + (time[7] - '0');

    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *p = strstr(months, monthStr);
    uint8_t month = p ? ((p - months) / 3 + 1) : 1;

    uint8_t yr = year % 100;

    

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);

    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));
    Wire.write(1); // DOW
    Wire.write(decToBcd(day));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(yr));

    Wire.endTransmission();

    // Clear OSF
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.write(0x00);
    Wire.endTransmission();

    
}