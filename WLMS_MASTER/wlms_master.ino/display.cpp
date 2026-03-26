#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        // Initialization failed
        while (1); // or log error
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.display();
}

void welcomeScreen() {
    display.clearDisplay();
    display.fillRect(0, 0, 128, 64, 1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 10);
    display.setTextSize(3);
    display.print("WLMS");
    display.setTextColor(BLACK, WHITE);
    display.setCursor(90, 23);
    display.setTextSize(1);
    display.print("v2.0");
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 45);
    display.print("Made By METECH");
    display.display();
}

void updateDisplay(uint8_t level, uint16_t voltage, uint8_t volume, const char* dateTime, bool isMotorOn = false, bool isWifiConnected = false, bool isMainsCut = false) {
    display.clearDisplay();
    //Water level indictor
    display.drawRect(108, 4, 20, 60, 1);
    display.drawLine(108, 58, 110, 58, 1);
    display.drawLine(108, 52, 110, 52, 1);
    display.drawLine(108, 46, 110, 46, 1);
    display.drawLine(108, 40, 110, 40, 1);
    display.drawLine(108, 34, 110, 34, 1);
    display.drawLine(108, 28, 110, 28, 1);
    display.drawLine(108, 22, 110, 22, 1);
    display.drawLine(108, 16, 110, 16, 1);
    display.drawLine(108, 10, 110, 10, 1);
    display.drawLine(108, 4, 110, 4, 1);

    display.setCursor(3, 3);
    display.setTextSize(1);
    display.print("Water LvL");
    display.setCursor(5, 15);
    display.setTextSize(2);
    display.printf("%d%%", level);
    display.drawLine(3, 31, 55, 31, 1);

    display.setTextSize(1);
    display.setCursor(60, 34);
    display.print("Voltage");
    display.setCursor(60, 44);
    display.printf("%dV", voltage);
    display.drawLine(55, 35, 55, 50, 1);

    display.setCursor(8, 34);
    display.print("Volume");
    display.setCursor(8, 44);
    display.printf("%dL", volume);
    display.drawLine(3, 53, 100, 53, 1);

    display.setCursor(5, 56);
    display.setTextSize(1);
    display.print(dateTime);

    // Status icons
    if(isMotorOn) {
        display.drawRect(60, 5, 45, 25, 1);
        display.drawCircle(72, 17, 7, 1);
        display.setCursor(70, 14);
        display.print("M");
    }
    if(!isMainsCut) {
        display.drawCircle(92, 17, 7, 1);
        display.setCursor(88, 14);
        display.setTextSize(2);
        display.print("~");
    }

    if(isWifiConnected) {
        display.fillTriangle(93, 63, 103, 63, 103, 55, 1);
        display.drawLine(102, 57, 102, 62, 0);
    }
    
    int bars = level / 5;
    int startY = 61;
    int step   = 3; 
    for (int i = 0; i < bars; i++) {
        int y = startY - (i * step);
        display.fillRect(109, y, 20, 3, 1);
    }
    display.display();
}