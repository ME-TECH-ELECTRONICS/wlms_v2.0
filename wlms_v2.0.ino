#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "rtc.h"

MyDS3231 rtc;
// ---------- DISPLAY SETUP (Page buffer mode) ----------
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---------- MOCK DATA (replace with real sensors) ----------
int level = 84;     // %
int voltage = 230;  // V
int volume = 1200;  // L

bool isMotorOn = true;
bool isMainsCut = false;
bool isWifiConnected = true;
const char* VERSION = "1.0.0";
char dateTime[20] = "03-04-26 11:30";

void welcomeScreen() {

  u8g2.firstPage();
  do {

    // ---- Background (white screen) ----
    u8g2.setDrawColor(1);         // 1 = draw pixels
    u8g2.drawBox(0, 0, 128, 64);  // filled white

    // ---- Invert text (black on white) ----
    u8g2.setDrawColor(0);  // 0 = clear pixels (black text)

    // ---- "WLMS" (large text) ----
    u8g2.setFont(u8g2_font_logisoso24_tr);  // similar to size 3
    u8g2.drawStr(15, 30, "WLMS");

    // ---- Version ----
    char buf[16];
    snprintf(buf, sizeof(buf), "v%s", VERSION);

    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(82, 30, buf);

    // ---- Footer ----
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(20, 55, "Made By METECH");
    u8g2.setDrawColor(1);

  } while (u8g2.nextPage());
}

// ---------- UI DRAW FUNCTION ----------
void drawUI() {
  u8g2.firstPage();
  do {
    // ---- Tank outline ----
    u8g2.drawFrame(108, 4, 20, 60);

    // ---- Smooth water fill ----
    int fillHeight = map(level, 0, 100, 0, 58);
    u8g2.drawBox(109, 63 - fillHeight, 18, fillHeight);

    // ---- Tank markers ----
    for (int y = 4; y < 60; y += 6) {
      u8g2.drawLine(108, y, 110, y);
    }

    // ---- Big percentage ----
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", level);
    u8g2.setFont(u8g2_font_logisoso16_tr);
    u8g2.drawStr(1, 20, buf);

    u8g2.drawLine(0, 25, 100, 25);

    u8g2.setFont(u8g2_font_logisoso16_tr);
    snprintf(buf, sizeof(buf), "%dV", voltage);
    u8g2.drawStr(60, 46, buf);

    u8g2.drawLine(55, 26, 55, 49);

    // ---- Volume ----
    u8g2.setFont(u8g2_font_logisoso16_tr);
    snprintf(buf, sizeof(buf), "%dL", volume);
    u8g2.drawStr(1, 46, buf);

    u8g2.drawLine(0, 50, 100, 50);

    // // ---- Date & Time ----
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(0, 62, dateTime);

    // ---- Motor status ----
    if (isMotorOn) {
      // u8g2.drawFrame(60, 5, 45, 25);
      u8g2.drawCircle(60, 10, 10);
      u8g2.drawStr(58, 15, "M");
    }

    // ---- Mains status (~) ----
    if (!isMainsCut) {
      u8g2.drawCircle(88, 10, 10);
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(83, 14, "AC");
    }

    // ---- WiFi icon ----
    if (isWifiConnected) {
      // u8g2.drawXBMP(95, 54, 8, 8, wifi_icon);
      u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
      u8g2.drawGlyph(95, 62, 0x00f8);  // star icon
    }
  } while (u8g2.nextPage());
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);
  rtc.begin();
  // rtc.setTimeFromCompile();

  for (int i = 0; VERSION[i] != '\0'; i++) {
    Serial.println((int)VERSION[i]);
  }
  u8g2.begin();
  welcomeScreen();
  delay(3000);
}

// ---------- LOOP ----------
void loop() {

  drawUI();
  delay(1000);
  char buf[20];
  rtc.formatDateTime(dateTime);

  // Serial.println(buf);  // Example: 04/04/26 11:30

  // ---- Demo animation (remove in real project) ----
  level += 2;
  if (level > 100) level = 0;
}