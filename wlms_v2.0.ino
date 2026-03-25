#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <SD.h>

// ================= CONFIG =================
#define MOTOR_PIN 4
#define BUTTON_START_PIN 16
#define BUTTON_MODE_PIN 17

#define START_THRESHOLD 10
#define STOP_THRESHOLD 90

#define VOLTAGE_MIN 220
#define VOLTAGE_FAIL 110

#define DAY_START_HOUR 6
#define DAY_END_HOUR 18

// ================= SYSTEM =================
enum Mode { MODE_AUTO,
            MODE_SEMI,
            MODE_MANUAL };
enum State {
  STATE_IDLE,
  STATE_WAIT_LOW,
  STATE_STARTING,
  STATE_RUNNING,
  STATE_BLOCKED,
  STATE_FULL,
  STATE_MANUAL
};

struct SystemState {
  float level;
  float voltage;
  bool motor;
  bool isDay;

  Mode mode;
  State state;

  int start_th;
  int stop_th;
};

SystemState sys;

// ================= RTOS =================
SemaphoreHandle_t sysMutex;
QueueHandle_t logQueue;

// ================= WEB =================
WebServer server(80);

// ================= MOTOR =================
void motor_init() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
}

void motor_on() {
  digitalWrite(MOTOR_PIN, HIGH);
}

void motor_off() {
  digitalWrite(MOTOR_PIN, LOW);
}

// ================= MOCK FUNCTIONS =================
// Replace these with real implementations

float read_voltage() {
  return random(200, 240);
}
float lora_receive() {
  static int val = 100;
  static bool decreasing = true;

  if (decreasing) {
    val--;
    if (val <= 9) {
      val = 9;
      decreasing = false;  // switch direction
    }
  } else {
    val++;
    if (val >= 100) {
      val = 100;
      decreasing = true;  // switch direction
    }
  }

  return (float)val;
}

bool is_daytime() {
  int h = (millis() / 1000) % 24;
  return (h >= DAY_START_HOUR && h <= DAY_END_HOUR);
}

// ================= LOGGER =================
struct LogMsg {
  char msg[128];
};

void log_event(const char *text) {
  LogMsg m;
  snprintf(m.msg, sizeof(m.msg), "%lu | %s\n", millis(), text);
  xQueueSend(logQueue, &m, 0);
}

void logger_task(void *pv) {
  LogMsg m;

  while (1) {
    if (xQueueReceive(logQueue, &m, portMAX_DELAY)) {
      Serial.println(m.msg);

      File f = SD.open("/log.txt", FILE_APPEND);
      if (f) {
        f.print(m.msg);
        f.close();
      }
    }
  }
}

// ================= CONTROL FSM =================
void control_task(void *pv) {
  while (1) {

    xSemaphoreTake(sysMutex, portMAX_DELAY);

    switch (sys.state) {

      case STATE_IDLE:
        sys.state = STATE_WAIT_LOW;
        break;

      case STATE_WAIT_LOW:
        if (sys.level <= sys.start_th && sys.mode == MODE_AUTO) {
          sys.state = STATE_STARTING;
          log_event("LOW LEVEL DETECTED");
        }
        break;

      case STATE_STARTING:
        if (sys.voltage >= VOLTAGE_MIN && sys.isDay) {
          motor_on();
          sys.motor = true;
          sys.state = STATE_RUNNING;
          log_event("MOTOR ON");
        } else {
          sys.state = STATE_BLOCKED;
          log_event("BLOCKED (LOW VOLT/NIGHT)");
        }
        break;

      case STATE_RUNNING:
        if (sys.voltage <= VOLTAGE_FAIL) {
          motor_off();
          sys.motor = false;
          sys.state = STATE_BLOCKED;
          log_event("VOLTAGE DROP - MOTOR OFF");
        } else if (sys.level >= sys.stop_th) {
          motor_off();
          sys.motor = false;
          sys.state = STATE_FULL;
          log_event("TANK FULL - MOTOR OFF");
        }
        break;

      case STATE_BLOCKED:
        if (sys.voltage >= VOLTAGE_MIN) {
          sys.state = STATE_STARTING;
          log_event("RETRY MOTOR");
        }
        break;

      case STATE_FULL:
        sys.state = STATE_WAIT_LOW;
        break;

      case STATE_MANUAL:
        break;
    }

    xSemaphoreGive(sysMutex);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ================= LORA TASK =================
void lora_task(void *pv) {
  while (1) {
    float lvl = lora_receive();

    xSemaphoreTake(sysMutex, portMAX_DELAY);
    sys.level = lvl;
    xSemaphoreGive(sysMutex);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ================= VOLTAGE TASK =================
void voltage_task(void *pv) {
  while (1) {
    float v = read_voltage();

    xSemaphoreTake(sysMutex, portMAX_DELAY);
    sys.voltage = v;
    sys.isDay = is_daytime();
    xSemaphoreGive(sysMutex);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ================= BUTTON TASK =================
void button_task(void *pv) {
  pinMode(BUTTON_START_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);

  bool lastStart = HIGH;
  bool lastMode = HIGH;

  while (1) {
    bool nowStart = digitalRead(BUTTON_START_PIN);
    bool nowMode = digitalRead(BUTTON_MODE_PIN);

    // Start button (active LOW)
    if (nowStart == LOW && lastStart == HIGH) {
      motor_on();

      xSemaphoreTake(sysMutex, portMAX_DELAY);
      sys.motor = true;
      sys.state = STATE_MANUAL;
      xSemaphoreGive(sysMutex);

      log_event("MANUAL MOTOR ON");
    }

    // Mode button
    if (nowMode == LOW && lastMode == HIGH) {
      xSemaphoreTake(sysMutex, portMAX_DELAY);
      sys.mode = (Mode)((sys.mode + 1) % 3);
      xSemaphoreGive(sysMutex);

      log_event("MODE CHANGED");
    }

    lastStart = nowStart;
    lastMode = nowMode;

    vTaskDelay(pdMS_TO_TICKS(50));  // debounce
  }
}

// ================= DISPLAY TASK =================
void display_task(void *pv) {
  while (1) {
    xSemaphoreTake(sysMutex, portMAX_DELAY);

    Serial.printf("Level: %.1f%% | Volt: %.1fV | Motor: %d | Mode: %d\n",
                  sys.level, sys.voltage, sys.motor, sys.mode);

    xSemaphoreGive(sysMutex);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ================= WEB =================
void handle_root() {
  String html = "<h1>Water Controller</h1>";
  html += "<p>Level: " + String(sys.level) + "%</p>";
  html += "<p>Voltage: " + String(sys.voltage) + "V</p>";
  html += "<p>Motor: " + String(sys.motor) + "</p>";
  server.send(200, "text/html", html);
}

void web_task(void *pv) {
  WiFi.softAP("ESP32_Controller");

  server.on("/", handle_root);
  server.begin();

  while (1) {
    server.handleClient();
    vTaskDelay(10);
  }
}

// ================= OTA =================
void ota_task(void *pv) {
  ArduinoOTA.begin();

  while (1) {
    ArduinoOTA.handle();
    vTaskDelay(10);
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  sys.start_th = START_THRESHOLD;
  sys.stop_th = STOP_THRESHOLD;
  sys.mode = MODE_AUTO;
  sys.state = STATE_IDLE;

  motor_init();

  sysMutex = xSemaphoreCreateMutex();
  logQueue = xQueueCreate(10, sizeof(LogMsg));

  SD.begin();

  xTaskCreatePinnedToCore(control_task, "control", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(lora_task, "lora", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(voltage_task, "volt", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(display_task, "display", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(logger_task, "logger", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(button_task, "button", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(web_task, "web", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(ota_task, "ota", 4096, NULL, 1, NULL, 0);
}

// ================= LOOP =================
void loop() {
  vTaskDelay(portMAX_DELAY);
}