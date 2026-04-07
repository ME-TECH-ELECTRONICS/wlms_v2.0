#pragma once
#include <Arduino.h>

struct RTCDateTime {
    uint8_t day, month, year;
    uint8_t hour, minute, second;
};

class MyDS3231 {
public:
    // 🔑 Access point
    static MyDS3231& getInstance() {
        static MyDS3231 instance;  // created once
        return instance;
    }

    // Delete copy/move
    MyDS3231(const MyDS3231&) = delete;
    MyDS3231& operator=(const MyDS3231&) = delete;

    // Public API
    void begin();
    bool isTimeValid();
    RTCDateTime getDateTime();
    void formatDateTime(const RTCDateTime& dt, char* buf);
    void setTimeFromCompile();

private:
    // 🔒 Private constructor
    MyDS3231() {}

    void clearOSF();
};