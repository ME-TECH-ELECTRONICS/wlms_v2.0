/* Include Libaray*/
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>

// Declare Global Variables and GPIO pins
const int AC_VOLTAGE_SENSOR_PIN 34
const int SD_CS_PIN 5
const int MOTOR_RELAY_PIN 32
const int AC_SUPPLY_LED_PIN 25
const int AUTO_MODE_LED_PIN 26
const int MOTOR_ON_LED_PIN 27
const int LOW_VOLTAGE_LED_PIN 14
const int BUZZER_PIN 13

bool acSupplyStatus = false;
bool motorStatus = false;
bool mode = true;
bool lowVoltage = false;
const byte RF_ADDR = "03152";
uint8_t level = 0;

const byte ant[] = {
    0x1f,
    0x15,
    0x0e,
    0x04,
    0x04,
    0x04,
    0x04,
    0x04
};
const byte sig1[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x05,
    0x09,
    0x11,
    0x1f
};
const byte sig2[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x05,
    0x09,
    0x19,
    0x1f
};
const byte sig3[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x05,
    0x0d,
    0x1d,
    0x1f
};
const byte sig4[] = {
    0x00,
    0x00,
    0x01,
    0x03,
    0x07,
    0x0f,
    0x1f,
    0x1f
};
const byte two[] = {
    0x00,
    0x00,
    0x00,
    0x0E,
    0x02,
    0x0E,
    0x08,
    0x0E
};
const byte gen[] = {
    0x00,
    0x00,
    0x00,
    0x0E,
    0x10,
    0x17,
    0x11,
    0x0E
};

File myFile;
RTC_DS1307 rtc;
RF24 radio(7, 8); // CE, CSN
LiquidCrystal_I2C lcd(0x27, 20, 4);
SoftwareSerial gsm(10, 11); // RX, TX

void beepBuzzer() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
    gsm.begin(9600); // Initialize gsm serial
    Serial.begin(115200);
    lcd.init();

    // Initialize GPIOs
    pinMode(AC_VOLTAGE_SENSOR_PIN, INPUT);
    pinMode(MOTOR_RELAY_PIN, OUTPUT);
    pinMode(AC_SUPPLY_LED_PIN, OUTPUT);
    pinMode(AUTO_MODE_LED_PIN, OUTPUT);
    pinMode(MOTOR_ON_LED_PIN, OUTPUT);
    pinMode(LOW_VOLTAGE_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Initialization failed!");
        while(1);
    }
    radio.openReadingPipe(0, RF_ADDR);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();
    lcdCustomCharInit();
}

void WriteToLCD(int lvl, float voltage, String datetime) {
    uint8_t range = calcRSSI();
    
}

uint8_t calcRSSI() {
    String resp = sendATCmd("AT+CSQ");
    int rssiIndex = resp.indexOf("+CSQ:") + 6;
    String rssiString = resp.substring(rssiIndex, response.indexOf(',', rssiIndex)); 
    int num = rssiString.toInt();
    if(num >= 20 && num <= 30) {
        return 5;
    } else if(num >= 15 && num <= 19) {
        return 4;
    } else if(num >= 10 && num <= 14) {
        return 3;
    } else if(num >= 2 && num <= 9) {
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
    lcd.createChar(6, two);
    lcd.createChar(7, gen);
}

String sendATCmd(String cmd) {
    gsm.println(cmd);
    sleep(1000);
    String response = "";
    while (gsm.available()) {
        char c = gsm.read();
        response += c;
    }
    return response;
}

void sleep(unsigned long delayMillis) {
    unsigned long startMillis = millis();
    unsigned long currentMillis = startMillis;
    unsigned long yieldInterval = 2500;
    unsigned long lastYieldMillis = startMillis;
    while (millis() - startMillis < delayMillis) {
        currentMillis = millis();
        if (delayMillis >= 10000 && currentMillis - lastYieldMillis >= yieldInterval) {
            yield();
            lastYieldMillis = currentMillis;
        }
    }
}