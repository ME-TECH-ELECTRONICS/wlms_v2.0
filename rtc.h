#pragma once
#include <Arduino.h>

struct RTCDateTime {
    uint8_t day, month, year;
    uint8_t hour, minute, second;
};

class MyDS3231 {
public:
    void begin();
    bool isTimeValid();
    // bool syncIfNeeded();  // auto OSF check + NTP sync
    RTCDateTime getDateTime();
    void formatDateTime(char* buf);
    void setTimeFromCompile();

private:
    void clearOSF();
    // void setDateTimeFromNTP();
};