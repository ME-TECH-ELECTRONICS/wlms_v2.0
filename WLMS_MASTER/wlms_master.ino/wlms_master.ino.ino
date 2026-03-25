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


void updateDisplay() {
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
    display.print("100%");
    display.drawLine(3, 31, 55, 31, 1);

    display.setTextSize(1);
    display.setCursor(60, 34);
    display.print("Voltage");
    display.setCursor(60, 44);
    display.print("230V");
    display.drawLine(55, 35, 55, 50, 1);

    display.setCursor(8, 34);
    display.print("Volume");
    display.setCursor(8, 44);
    display.print("1000L");
    display.drawLine(3, 53, 100, 53, 1);

    // Status icons
    display.drawRect(60, 5, 45, 25, 1);
    display.drawCircle(72, 17, 7, 1);
    display.setCursor(70, 14);
    display.print("M");
    display.drawCircle(92, 17, 7, 1);
    display.setCursor(88, 14);
    display.setTextSize(2);
    display.print("~");
    
    display.setCursor(10, 56);
    display.setTextSize(1);
    display.print("12/12/25 12:25");
    
    display.fillRect(109, 58, 20, 6, 1);
    
    display.display();
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
