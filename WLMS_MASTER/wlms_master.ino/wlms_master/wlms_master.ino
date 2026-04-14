#include "rtc.h"

MyDS3231& rtc = MyDS3231::getInstance();

void setup() {
    Serial.begin(115200);
    rtc.begin();

    if (!rtc.isTimeValid()) {
        Serial.println("RTC time invalid, setting from compile time.");
        rtc.setTimeFromCompile();
    }
}

void loop() {
    RTCDateTime dt = rtc.getDateTime();
    char buf[20];
    rtc.formatDateTime(dt, buf);
    Serial.println(buf);
    delay(1000);
}