/* Include Libaray */
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <RF24.h>
#include <Wire.h>
#include <RTClib.h>
#include <Update.h>
#include <nRF24L01.h>
#include <ZMPT101B.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>

/* Declare Global Variables and GPIO pins */
const int AC_VOLTAGE_SENSOR_PIN = 34;
const int SD_CS_PIN = 13;
const int RF_CS_PIN = 5;
const int MOTOR_RELAY_PIN = 27;
const int MANUAL_START_BTN = 26;
const int AUTO_MODE_LED_PIN = 15;
const int BUZZER_PIN = 12;
const int ROTARY_SW_PIN = 25;

bool motorStatus = false;
bool mode = false;
bool resumeOnACrestore = false;
const byte RF_ADDR[] = "03152";

uint8_t level = 0; //thus vayiable need to be changed with struct

float acVoltage = 0;
unsigned long startTime;
uint16_t elapsed = 0;
uint8_t trig_rate = 0;
const char *ssid = "WLMS 2.0";
const char *password = "wlms@2024";

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

struct RxData {
    int distance;
};

const uint8_t L[4][21] = {
    {
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        4,
        4,
        2,
        3,
        2,
        3,
        2,
        4,
        2,
        3,
        3
    },
    {
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        4,
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        2,
        1,
        1
    },
    {
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        4,
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        4,
        4,
        2
    },
    {
        4,
        2,
        1,
        2,
        1,
        2,
        4,
        2,
        1,
        1,
        4,
        2,
        4,
        2,
        4,
        2,
        4,
        1,
        1,
        2
    }
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

byte full[] = {
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F
};

byte btHalf[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x1F,
    0x1F,
    0x1F,
    0x1F
};
byte tpHalf[] = {
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x00,
    0x00,
    0x00,
    0x00
};
byte blank[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};

File dataFile;
RTC_DS1307 rtc;
DataRecord record = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};
RxData data;
RF24 radio(7, RF_CS_PIN); // CE, CSN
LiquidCrystal_I2C lcd(0x27, 20, 4);
ZMPT101B vSense(AC_VOLTAGE_SENSOR_PIN, 50.0);
WebServer server(80);
SemaphoreHandle_t lcdWrite;
SemaphoreHandle_t dataRW;

void webServerTask(void *pvParameters);
void handleBtn(void *pvParameters);
void mainLoop(void *pvParameters);

void setup() {
    Serial.begin(115200);
    lcd.createChar(0, wifi);
    lcd.createChar(1, btHalf);
    lcd.createChar(2, full);
    lcd.createChar(3, tpHalf);
    lcd.createChar(4, blank);
    analogReadResolution(12);

    // Initialize GPIOs
    initializePins();

    lcdWrite = xSemaphoreCreateMutex();
    dataRW = xSemaphoreCreateMutex();

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Initialization failed!");
        while (1);
    }

    dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
        if (dataFile.size() == 0) {
            dataFile.println("Sl_No,Water_Level_ON,Water_Level_OFF,Motor_ON,Motor_OFF,Voltage,Mode,Remark");
        }
        dataFile.close();
    }
    if(!rtc.begin()) Serial.print("Couldn't find RTC");
    radio.openReadingPipe(0, RF_ADDR);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();
    xTaskCreate(webServerTask, "WebServerTask", 8192, NULL, 1, NULL);
    xTaskCreate(mainLoop, "mainLoop", 2048, NULL, 1, NULL);
    xTaskCreate(handleBtn, "handleBtn", 1024, NULL, 1, NULL);
    vSense.setSensitivity(500.0);
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
    Serial.println("Successfully Initialized");
    beepBuzzer();
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 4; j++) {
            lcd.setCursor(i, j);
            lcd.write(L[j][i]);
            delay(5);
        }
    }
    delay(2500);
    for(int x = 0; x < 20; x++) {
        for(int y = 0; y < 4; y++) {
            lcd.setCursor(x, y);
            lcd.write(4);
            delay(5);
        }
    }
}

void mainLoop(void *pvParameters) {
    uint8_t pipe;
    while(1) {
        if (radio.available(&pipe)) {
            radio.read(&data, sizeof(data));
        }
        acVoltage = vSense.getRmsVoltage(5);
        DateTime now = rtc.now();
        if((level <= 15) && (acVoltage > 220) && (mode == false)) {
            startTime = millis();
            beepBuzzer();
            digitalWrite(MOTOR_RELAY_PIN, HIGH);
            trig_rate++;
            motorStatus = true;
            record.slNo += 1;
            record.onTime = now.unixtime();
            record.onWaterLvl = level;
            record.acVoltage = acVoltage;
            record.modeState = mode;
        }
        if((level < 100) && (resumeOnACrestore == true) && (acVoltage > 220)) {
            resumeOnACrestore = false;
            trig_rate++;
            digitalWrite(MOTOR_RELAY_PIN, HIGH);
            startTime = millis();
            record.slNo += 1;
            record.onWaterLvl = level;
            record.acVoltage = acVoltage;
            record.modeState = mode;
            record.onTime = now.unixtime();
        }
        if(acVoltage < 220) {
            if(xSemaphoreTake(dataRW, portMAX_DELAY) == pdTRUE) {
                elapsed = millis() - startTime;
                resumeOnACrestore = true;
                motorCtrl_Logging(1);
                xSemaphoreGive(dataRW);
            }
        }

        if(level > 99) {
            digitalWrite(MOTOR_RELAY_PIN, LOW);
            if(xSemaphoreTake(dataRW, portMAX_DELAY) == pdTRUE) {
                elapsed = millis() - startTime;
                motorCtrl_Logging(0);
                xSemaphoreGive(dataRW);
            }
        }
        WriteToLCD(level, acVoltage, mode, motorStatus);
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
void handleBtn(void *pvParameters) {
    while(1) {
        if(digitalRead(MANUAL_START_BTN) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(20));
            motorStatus = !motorStatus;
            digitalWrite(MOTOR_RELAY_PIN, motorStatus ? HIGH: LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
void webServerTask(void *pvParameters) {
    while (true) {
        server.handleClient();
        vTaskDelay(1); // Yield to other tasks
    }
}

void initializePins() {
    int inputPins[] = {
        AC_VOLTAGE_SENSOR_PIN,
        MANUAL_START_BTN,
        ROTARY_SW_PIN
    };
    for (int i = 0; i < 3; i++) {
        pinMode(inputPins[i], INPUT);
    }
    int outputPins[] = {
        MOTOR_RELAY_PIN,
        AUTO_MODE_LED_PIN,
        BUZZER_PIN
    };
    for (int i = 0; i < 3; i++) {
        pinMode(outputPins[i], OUTPUT);
    }
}

void beepBuzzer() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
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
        String jsonData = "{\"motor_status\": " + String(motorStatus) + ", \"ctrl_mode\": " + String(mode) + ", \"water_lvl\": " + String(level) + ", \"voltage\": " + String(acVoltage) + ", \"motor_ontime\": " + String(elapsed) + ", \"trig_rate\": " + String(trig_rate) + "}";
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


void loop() {}
void motorCtrl_Logging(uint8_t remark) {
    DateTime now = rtc.now();
    digitalWrite(MOTOR_RELAY_PIN, LOW);
    record.offWaterLvl = level;
    record.offTime = now.unixtime();
    record.remark = remark;
    vTaskDelay(pdMS_TO_TICKS(1));
    logData();
}
void WriteToLCD(uint8_t lvl, int voltage, uint8_t m, uint8_t motor) {
    DateTime now = rtc.now();
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
    lcd.print(formatDate(now));
    lcd.setCursor(13, 3);
    lcd.print(formatTime(now));
}

void logData() {
    dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
        dataFile.print(record.slNo);
        dataFile.print(",");
        dataFile.print(record.onWaterLvl);
        dataFile.print(",");
        dataFile.print(record.offWaterLvl);
        dataFile.print(",");
        dataFile.print(record.onTime);
        dataFile.print(",");
        dataFile.print(record.offTime);
        dataFile.print(",");
        dataFile.print(record.acVoltage);
        dataFile.print(",");
        dataFile.print(mode);
        dataFile.print(",");
        dataFile.println(record.remark);
        dataFile.close();
    }
}


String formatTime(DateTime now) {
    uint8_t hour = now.hour();
    uint8_t minute = now.minute();
    bool isPM = (hour > 12) ? true: false;
    if(hour > 12) hour -= 12;
    char timeStr[11];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d%s", hour, minute, isPM ? "PM": "AM");
    return String(timeStr);
}

String formatDate(DateTime now) {
    uint8_t day = now.day();
    uint8_t month = now.month();
    uint8_t year = now.year();
    char dateStr[16];
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%02d", day, month, year % 100);
    return String(dateStr);
}