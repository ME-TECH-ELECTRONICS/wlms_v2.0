const uint8_t MOTOR = 15;
const uint8_t MOTOR_STATUS_LED = 33;
const uint8_t MODE_LED_RED = 25;
const uint8_t MODE_LED_GREEN = 26;
const uint8_t MODE_BTN = 27;
const uint8_t MANUAL_BTN = 14;
const uint8_t VOLTAGE_SENS = 13;
const uint8_t LORA_RST = 16;
const uint8_t LORA_D0 = 4;
const uint8_t LORA_CHIP_SELECT = 5;
const uint8_t SDCARD_CHIP_SELECT = 17;


uint8_t MOTOR_START_THRESHOLD = 10;
uint8_t MOTOR_STOP_THRESHOLD = 100;

uint8_t VOLTAGE_MIN = 220;
uint8_t VOLTAGE_MAX = 250;

uint8_t DAY_START_HOUR = 6;
uint8_t DAY_END_HOUR = 18;

enum Mode {
    MODE_AUTO,
    MODE_SEMI_AUTO,
    MODE_MANUAL
}

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
SemaphoreHandle_t sysMutex;
QueueHandle_t logQueue;

void control_task(void *pv) {
    while (1) {
        
    }
    
}

void setup() {
    pinMode(MODE_BTN, INPUT_PULLUP);
    pinMode(MANUAL_BTN, INPUT_PULLUP);
    pinMode(MOTOR, OUTPUT);
    pinMode(MOTOR_STATUS_LED, OUTPUT);
    pinMode(MODE_LED_RED, OUTPUT);
    pinMode(MODE_LED_GREEN, OUTPUT);
    pinMode(VOLTAGE_SENS, INPUT);
    digitalWrite(MOTOR, HIGH);
    digitalWrite(MOTOR_STATUS_LED, LOW);
    digitalWrite(MODE_LED_RED, LOW);
    digitalWrite(MODE_LED_GREEN, HIGH);
}

void loop() {
    // put your main code here, to run repeatedly:
}
