/* Include Libaray */
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <HardwareSerial.h>
#include <ZMPT101B.h>

// Declare Global Variables and GPIO pins
const int AC_VOLTAGE_SENSOR_PIN = 34;
const int SD_CS_PIN = 5;
const int MOTOR_RELAY_PIN = 32;
const int AC_SUPPLY_LED_PIN = 25;
const int AUTO_MODE_LED_PIN = 26;
const int MOTOR_ON_LED_PIN = 27;
const int LOW_VOLTAGE_LED_PIN = 14;
const int BUZZER_PIN = 13;

bool acSupplyStatus = false;
bool motorStatus = false;
bool mode = true;
bool lowVoltage = false;
const byte RF_ADDR[] = "03152";
uint8_t level = 0;
uint16_t acVoltage = 0;
uint8_t prevRow = 1;
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

byte ant[] = {
    0x1f,
    0x15,
    0x0e,
    0x04,
    0x04,
    0x04,
    0x04,
    0x04
};
byte sig1[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x05,
    0x09,
    0x11,
    0x1f
};
byte sig2[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x05,
    0x09,
    0x19,
    0x1f
};
byte sig3[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x05,
    0x0d,
    0x1d,
    0x1f
};
byte sig4[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x07,
    0x0f,
    0x1f,
    0x1f
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
HardwareSerial gsm(2);
ZMPT101B vSense(A0, 50.0);


void beepBuzzer() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
    gsm.begin(9600); // Initialize gsm serial
    Serial.begin(115200);
    lcd.init();
    analogReadResolution(12);
    // Initialize GPIOs
    pinMode(AC_VOLTAGE_SENSOR_PIN, INPUT);
    pinMode(MOTOR_RELAY_PIN, OUTPUT);
    pinMode(AC_SUPPLY_LED_PIN, OUTPUT);
    pinMode(AUTO_MODE_LED_PIN, OUTPUT);
    pinMode(MOTOR_ON_LED_PIN, OUTPUT);
    pinMode(LOW_VOLTAGE_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);


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
}

void mainLoop(void *pvParameters) {
  uint8_t pipe;
    if (radio.available(&pipe)) {              // is there a payload? get the pipe number that recieved it
      uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
      radio.read(&level, bytes);  
    }
    DateTime now = rtc.now();
    if((level <= 15) && (acVoltage > 220) && (mode == false)) {
        digitalWrite(MOTOR_RELAY_PIN, HIGH);
        record.slNo += 1;
        record.onTime = now.unixtime();
        record.onWaterLvl = level;
        record.acVoltage = acVoltage;
        record.modeState = mode;

    } 
    if(acVoltage < 220) {
        digitalWrite(MOTOR_RELAY_PIN, LOW);
        record.offWaterLvl = level;
        record.offTime = now.unixtime();
        record.remark = 1;
        vTaskDelay(pdMS_TO_TICKS(1000));
        logData();
    }
    
    if(level > 99) {
        digitalWrite(MOTOR_RELAY_PIN, LOW);
        record.offTime = now.unixtime();
        record.remark = 0;
        vTaskDelay(pdMS_TO_TICKS(1000));
        logData();
    }
}
void loop() {
    float voltage = vSense.getRmsVoltage();
}
void WriteToLCD(uint8_t lvl, int voltage, uint8_t mode, String date, String tim, uint8_t motor) {
    uint8_t range = calcRSSI();
    //Line 1
    lcd.setCursor(0, 0);
    lcd.print("WLMS v2.0");
    lcd.setCursor(17, 0);
    lcd.print((mode == 0) ? "M": "A");
    lcd.write(1);
    lcd.write(range);

    //Line 2
    lcd.setCursor(0, 1);
    lcd.print("LVL: ");
    lcd.print(lvl);
    lcd.print("%  MOTOR ");
    lcd.print((motor == 0) ? "OFF": "ON");

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

uint8_t calcRSSI() {
    String resp = sendATCmd("AT+CSQ");
    int rssiIndex = resp.indexOf("+CSQ:") + 6;
    String rssiString = resp.substring(rssiIndex, resp.indexOf(',', rssiIndex));
    int num = rssiString.toInt();
    if (num >= 20 && num <= 30) {
        return 5;
    } else if (num >= 15 && num <= 19) {
        return 4;
    } else if (num >= 10 && num <= 14) {
        return 3;
    } else if (num >= 2 && num <= 9) {
        return 2;
    } else {
        return 0;
    }
}

void lcdCustomCharInit() {
    lcd.createChar(1, ant);
    lcd.createChar(2, sig1);
    lcd.createChar(3, sig2);
    lcd.createChar(4, sig3);
    lcd.createChar(5, sig4);
}

String sendATCmd(String cmd) {
    gsm.println(cmd);
    vTaskDelay(pdMS_TO_TICKS(1000));
    String response = "";
    while (gsm.available()) {
        char c = gsm.read();
        response += c;
    }
    return response;
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