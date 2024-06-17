/* Include Libaray */
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <WebServer.h>
#include <FS.h>
#include <Update.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ZMPT101B.h>

/* Declare Global Variables and GPIO pins */
const int AC_VOLTAGE_SENSOR_PIN = 34;
const int SD_CS_PIN = 5;
const int MOTOR_RELAY_PIN = 32;
const int AC_SUPPLY_LED_PIN = 25;
const int AUTO_MODE_LED_PIN = 26;
const int MOTOR_ON_LED_PIN = 27;
const int LOW_VOLTAGE_LED_PIN = 14;
const int BUZZER_PIN = 13;
const int ROTARY_CLK_PIN = 2;
const int ROTARY_DT_PIN = 3;
const int ROTARY_SW_PIN = 3;

bool motorStatus = false;
const byte RF_ADDR[] = "03152";
uint8_t level = 0;
float acVoltage = 0;
unsigned long startTime;
uint16_t elapsed = 0;
uint8_t trig_rate = 0;
uint8_t prevRow = 1;
uint8_t count = 1;
uint8_t clkState;
uint8_t clkLastState;
String systemMenuList[2] = {
    "Mode",
    "Logging"
};
String TimeMenuList[2] = {
    "Set Date",
    "Set Time"
};
String NetworkMenuList[2] = {
    "Update Rate",
    "Network Type"
};
struct defaultSettings {
    bool mode;
    bool logging;
    String setDate;
    String setTime;
    uint8_t updateRate;
    bool networkType;
};

struct DataRecord {
    uint16_t slNo;
    uint8_t onWaterLvl;
    uint8_t offWaterLvl;
    uint32_t onTime;
    uint32_t offTime;
    uint16_t acVoltage;
    uint8_t modeState;
    uint8_t remark;
};

byte wifi[] = {
    0x00,
    0x0E,
    0x11,
    0x00,
    0x04,
    0x0A,
    0x00,
    0x04
};


File dataFile;
RTC_DS1307 rtc;
DataRecord record;
defaultSettings settings = {
    false,
    true,
    "00/00/0000",
    "00:00AM",
    2,
    false
};
RF24 radio(7, 8); // CE, CSN
LiquidCrystal_I2C lcd(0x27, 20, 4);
ZMPT101B vSense(A0, 50.0);
WebServer server(80);
SemaphoreHandle_t lcdWrite;

void beepBuzzer() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
    Serial.begin(115200);
    lcd.createChar(0, wifi);
    analogReadResolution(12);
    // Initialize GPIOs
    pinMode(AC_VOLTAGE_SENSOR_PIN, INPUT);
    pinMode(ROTARY_CLK_PIN, INPUT);
    pinMode(ROTARY_DT_PIN, INPUT);
    pinMode(MOTOR_RELAY_PIN, OUTPUT);
    pinMode(AC_SUPPLY_LED_PIN, OUTPUT);
    pinMode(AUTO_MODE_LED_PIN, OUTPUT);
    pinMode(MOTOR_ON_LED_PIN, OUTPUT);
    pinMode(LOW_VOLTAGE_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    lcdWrite = xSemaphoreCreateMutex();
    clkLastState = digitalRead(ROTARY_CLK_PIN);
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Initialization failed!");
        while (1);
    }

    dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
        if (dataFile.size() == 0) {
            dataFile.println("Sl_No,Water_Level,Motor_ON,Motor_OFF,Voltage,Mode,Remark");
        }
        dataFile.close();
    }
    if(!rtc.begin()) Serial.print("Couldn't find RTC");
    radio.openReadingPipe(0, RF_ADDR);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();
    lcdCustomCharInit();

    vSense.setSensitivity(500.0);
    Serial.println("successful Initilized");
    WiFi.softAP(ssid, password);
    Serial.println("Access Point created");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/reqData", handleReqData);
    server.on("/update", HTTP_GET, handleOTA); // OTA page
    server.on("/upload", HTTP_POST, []() {
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL": "OK");
        ESP.restart();
    }, handleOTAUpdate);
    server.begin();
}

void mainLoop(void *pvParameters) {
    uint8_t pipe;
    while(1) {
        if (radio.available(&pipe)) {
            uint8_t bytes = radio.getPayloadSize();
            radio.read(&level, bytes);
        }
        acVoltage = vSense.getRmsVoltage(5);
        DateTime now = rtc.now();
        if((level <= 15) && (acVoltage > 220) && (settings.mode == false)) {
            startTime = millis();
            digitalWrite(MOTOR_RELAY_PIN, HIGH);
            motorStatus = true;
            record.slNo += 1;
            record.onTime = now.unixtime();
            record.onWaterLvl = level;
            record.acVoltage = acVoltage;
            record.modeState = settings.mode;

        }
        if(acVoltage < 220) {
            digitalWrite(MOTOR_RELAY_PIN, LOW);
            if(xSemaphoreTake(dataRW, portMAX_DELAY) == pdTRUE) {
                elapsed = millis() - startTime;
                record.offWaterLvl = level;
                record.offTime = now.unixtime();
                record.remark = 1;
                vTaskDelay(pdMS_TO_TICKS(1));
                logData();
                xSemaphoreGive(dataRW);
            }
        }

        if(level > 99) {
            digitalWrite(MOTOR_RELAY_PIN, LOW);
            if(xSemaphoreTake(dataRW, portMAX_DELAY) == pdTRUE) {
                elapsed millis() - startTime;
                record.offTime = now.unixtime();
                record.remark = 0;
                vTaskDelay(pdMS_TO_TICKS(1));
                logData();
                xSemaphoreGive(dataRW);
            }
        }
        if(xSemaphoreTake(lcdWrite, portMAX_DELAY) == pdTRUE) {
            WriteToLCD(level, acVoltage, settings.mode, dateNow, timeNow, motorStatus);
            xSemaphoreGive(lcdWrite);
            vTaskDelay(pdMS_TO_TICKS(25));
        }
    }
}
void webServerTask(void *pvParameters) {
    while (true) {
        server.handleClient();
        vTaskDelay(1); // Yield to other tasks
    }
}

void dataGenerationTask(void *pvParameters) {
    while (true) {
        count++;
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Increment count every 5 seconds
    }
}

void menuPrint(void *pvParameters) {
    uint8_t menuDepth = 1;
    uint8_t menu = 1;
    while(1) {
        uint8_t enterBtnState = digitalRead(enterBtn);
        if(enterBtnState == LOW) {
            while(!enterBtnState);
            while(xSemaphoreTake(lcdWrite, portMAX_DELAY) == pdTRUE) {
                MainMenu();
                if(digitalRead(escBtn) == LOW) {
                    xSemaphoreGive(lcdWrite);
                    break;
                }
                readRotary(3);
                if(digitalRead(ROTARY_SW_PIN) == LOW) {
                    if(prevRow == 1) {
                        subMenu(systemMenuList, "MENU > SYSTEM", 2);
                        menuDepth++;
                        while(1) {
                            if(digitalRead(ESC_BTN_PIN) == LOW) {
                                menuDepth--;
                                break;
                            };
                            readRotary(2);
                            if(prevRow == 1) {
                                subMenu(["Automatic", "Manual"], "SYSTEM > MODE", 2);
                                menuDepth++;
                                while(1) {
                                    readRotary();
                                    if(digitalRead(ROTARY_SW_PIN == LOW)) {
                                        settings.mode = (prevRow == 1) ? false: true;
                                    }
                                }
                            }

                        }
                    } else if(prevRow = 2) {
                        subMenu(TimeMenuList, "MENU > TIME", 2);
                        menuDepth++;
                        menu = prevRow;
                    } else {
                        subMenu(NetworkMenuList, "MENU > NETWORK", 2);
                        menuDepth++;
                        menu = prevRow;
                    }
                    while(1) {
                        if(digitalRead(ESC_BTN_PIN) == LOW) break;

                    }

                }
            }
        }
    }
}

void handleRoot() {
    File file = SD.open("/index.html");
    if (!file) {
        server.send(500, "text/plain", "File not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleReqData() {
    if(xSemaphoreTake(dataRW, portMAX_DELAY) == pdTRUE) {
        String jsonData = "{\"motor_status\": " + String(motorStatus) + ", \"ctrl_mode\": " + String(settings.mode) + ", \"water_lvl\": " + String(level) + ", \"voltage\": " + String(acVoltage) + ", \"motor_ontime\": " + String(elapsed) + ", \"trig_rate\": " + String(trig_rate) + "}";
        server.send(200, "application/json", jsonData);
        xSemaphoreGive(dataRW);
    }
}

void handleOTA() {
    File file = SD.open("/update.html");
    if (!file) {
        server.send(500, "text/plain", "OTA page not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleOTAUpdate() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}

void readRotary(uint8_t maxPoint) {
    clkState = digitalRead(ROTARY_CLK_PIN);
    if(clkState != clkLastState) {
        if(digitalRead(ROTARY_DT_PIN) != clkState) {
            count++;
        } else {
            count--;
        }
        if(count > maxPoint) count = maxPoint;
        if(count > 1) count = 1;
    }
    moveCursor(count);

    clkLastState = clkState;
}
void loop() {}
void WriteToLCD(uint8_t lvl, int voltage, uint8_t m, String date, String tim, uint8_t motor) {
    //Line 1
    lcd.setCursor(0, 0);
    lcd.print("WLMS v2.0");
    lcd.setCursor(18, 0);
    lcd.print((m == 0) ? "M": "A");
    lcd.write(0);

    //Line 2
    lcd.setCursor(0, 1);
    lcd.print("LVL: ");
    lcd.print(lvl);
    lcd.print("%  MOTOR ");
    lcd.print((motor == false) ? "OFF": "ON");

    //Line 3
    lcd.setCursor(0, 2);
    lcd.print("MAINS: ");
    lcd.print(voltage);

    //Line 4
    lcd.setCursor(0, 3);
    lcd.print(date);
    lcd.setCursor(13, 3);
    lcd.print(tim);
}

bool logData() {
    if (dataFile) {
        dataFile.print(record.slNo);
        dataFile.print(",");
        dataFile.print(record.waterLvl);
        dataFile.print(",");
        dataFile.print(record.onTime);
        dataFile.print(",");
        dataFile.print(record.offTime);
        dataFile.print(",");
        dataFile.print(record.acVoltage);
        dataFile.print(",");
        dataFile.print(record.motorStatus);
        dataFile.print(",");
        dataFile.println(record.Remark);
        dataFile.close();
    }
    return false;
}


String formatTime() {
    uint8_t hour = rtc.hour();
    uint8_t minute = rtc.minute();
    bool isPM = (rtc.hourModeAndAmPm() == 2) ? true: false;
    char timeStr[8];

    snprintf(timeStr, sizeof(timeStr), "%02d:%02d%s", hour, minute, isPM ? "PM": "AM");
    return timeStr;
}

/*#include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27, 20, 4);
  uint8_t prevRow = 1;
  void setup() {
    lcd.init();
    MainMenu();
    delay(1000);
    moveCursor(2);
    delay(1000);
    moveCursor(3);
    delay(1000);
    moveCursor(2);
    delay(1000);
    moveCursor(1);
  }*/

void loop() {}


void MainMenu() {
    lcd.setCursor(0, 0);
    lcd.print("MENU");
    lcd.setCursor(0, 1);
    lcd.print("> System");
    lcd.setCursor(2, 2);
    lcd.print("Time");
    lcd.setCursor(2, 3);
    lcd.print("Network");
}

void subMenu(String a[], String title, uint8_t length) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(title);
    for (int i = 0; i < length; i++) {
        lcd.setCursor(2, i + 1);
        lcd.print(a[i]);
    }
    lcd.setCursor(0, 1);
    lcd.print(">");
}
void moveCursor(uint8_t row) {
    lcd.setCursor(0, prevRow);
    lcd.print(" ");
    lcd.setCursor(0, row);
    lcd.print(">");
    prevRow = row;
}