/* Include Libaray*/
#include <WiFi.h>
#include <esp_now.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// Declare Global Variables and GPIO pins
const int AC_VOLTAGE_SENSOR_PIN 34
const int SD_CS_PIN 5
const int MOTOR_RELAY_PIN 32
const int AC_SUPPLY_LED_PIN 25
const int AUTO_MODE_LED_PIN 26
const int MOTOR_ON_LED_PIN 27
const int LOW_VOLTAGE_LED_PIN 14
const int BUZZER_PIN 13


File myFile;

