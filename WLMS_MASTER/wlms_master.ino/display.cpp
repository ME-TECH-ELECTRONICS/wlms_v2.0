#include <stdint.h>
#include "display.h"
#include <U8g2lib.h>
#include "config.h"
#include "system.h"
#include "rtc.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
MyDS3231& rtc = MyDS3231::getInstance();

inline char* utoa_fast(uint32_t val, char* buf) {
  char* p = buf;

  if (val == 0) {
    *p++ = '0';
  } else {
    char tmp[10];
    int i = 0;

    while (val) {
      tmp[i++] = '0' + (val % 10);
      val /= 10;
    }

    while (i--) {
      *p++ = tmp[i];
    }
  }

  *p = '\0';
  return p;  // returns end pointer
}

void initDisplay() {
  if (!display.begin()) {
    while (1)
      ;
  }
}

void welcomeScreen() {
  display.firstPage();
  do {
    display.setDrawColor(1);
    display.drawBox(0, 0, 128, 64);
    display.setDrawColor(0);
    display.setFont(u8g2_font_logisoso24_tr);  // similar to size 3
    display.drawStr(15, 30, "WLMS");
    char buf[16];
    snprintf(buf, sizeof(buf), "v%s", VERSION);
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(82, 30, buf);
    display.setFont(u8g2_font_6x13_tr);
    display.drawStr(20, 55, "Made By METECH");
    display.setDrawColor(1);
  } while (display.nextPage());
}

void updateDisplay(uint8_t level, uint16_t voltage, uint16_t volume, const char* dateTime, bool isMotorOn, bool isWifiConnected, bool isMainsCut) {
  // Serial.printf("L:%d V:%d Vol:%d Time:%s Motor:%d WiFi:%d Mains:%d\n", level, voltage, volume, dateTime, isMotorOn, isWifiConnected, isMainsCut);
  display.firstPage();
  do {
    display.drawFrame(108, 4, 20, 60);
    int fillHeight = map(level, 0, 100, 0, 58);
    display.drawBox(109, 63 - fillHeight, 18, fillHeight);
    for (int y = 4; y < 60; y += 6) {
      display.drawLine(108, y, 110, y);
    }
    char buf[12];
    char* p = utoa_fast(level, buf);
    *p++ = '%';
    *p = '\0';
    display.setFont(u8g2_font_logisoso16_tr);
    display.drawStr(1, 20, buf);
    display.drawLine(0, 25, 100, 25);
    p = utoa_fast(voltage, buf);
    *p++ = 'V';
    *p = '\0';
    display.drawStr(60, 46, buf);
    display.drawLine(55, 26, 55, 49);
    p = utoa_fast(volume, buf);
    *p++ = 'L';
    *p = '\0';
    display.drawStr(1, 46, buf);
    display.drawLine(0, 50, 100, 50);
    display.setFont(u8g2_font_6x13_tr);
    display.drawStr(0, 62, dateTime);
    if (isMotorOn) {
      display.drawCircle(60, 10, 10);
      display.drawStr(58, 15, "M");
    }
    if (!isMainsCut) {
      display.drawCircle(88, 10, 10);
      display.setFont(u8g2_font_6x10_tr);
      display.drawStr(83, 14, "AC");
    }
    if (isWifiConnected) {
      display.setFont(u8g2_font_open_iconic_all_1x_t);
      display.drawGlyph(95, 62, 0x00f8);
    }
  } while (display.nextPage());
}

void displayTask(void* pv) {
  while (1) {

    SystemState local;

    // ---- SAFE STATE COPY ----
    if (xSemaphoreTake(sysMutex, pdMS_TO_TICKS(50))) {
      local = sys;
      xSemaphoreGive(sysMutex);
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // ---- RTC READ (already mutex protected internally) ----
    RTCDateTime dt = rtc.getDateTime();

    char buf[20];
    rtc.formatDateTime(dt, buf);

    // ---- DISPLAY UPDATE ----
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {

      updateDisplay(local.level, local.voltage, local.level * 10, buf, local.motor, local.isWifiConnected, local.isMainsCut);

      xSemaphoreGive(i2cMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}