#define TRIG_PIN 7
#define ECHO_PIN 8

#define TANK_HEIGHT 100.0
#define SENSOR_OFFSET 2.0

#define SAMPLE_COUNT 5

unsigned long previousMillis = 0;
const unsigned long interval = 300;

static float samples[SAMPLE_COUNT];

// trend tracking
static int lastPercent = -1;

// Kalman variables
float kalman_x = 0;
float kalman_p = 1;

const float kalman_q = 0.1;
const float kalman_r = 5.0;

bool kalmanInitialized = false;

unsigned long lastPrintTime = 0;
unsigned long lastFailPrint = 0;

// ---------------- SENSOR ----------------

void triggerSensor() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}

float readDistance() {
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.0343 / 2.0;
}

// ---------------- KALMAN ----------------

float kalmanFilter(float measurement) {
  kalman_p += kalman_q;
  float k = kalman_p / (kalman_p + kalman_r);

  kalman_x = kalman_x + k * (measurement - kalman_x);
  kalman_p = (1 - k) * kalman_p;

  return kalman_x;
}

// ---------------- MEDIAN ----------------

float getMedian(float *arr, int size) {
  float a[SAMPLE_COUNT];

  for (int i = 0; i < size; i++) {
    a[i] = arr[i];
  }

  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (a[j] < a[i]) {
        float t = a[i];
        a[i] = a[j];
        a[j] = t;
      }
    }
  }

  if (size % 2 == 0)
    return (a[size / 2 - 1] + a[size / 2]) / 2.0;
  else
    return a[size / 2];
}

// ---------------- CHECKSUM ----------------

uint8_t calculateChecksum(uint8_t *data, uint8_t length) {
  uint8_t checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("System started...");
}

// ---------------- LOOP ----------------

void loop() {
  unsigned long now = millis();

  if (now - previousMillis >= interval) {
    previousMillis = now;

    int validSamples = 0;

    // -------- SAMPLING --------
    for (int i = 0; i < SAMPLE_COUNT; i++) {

      triggerSensor();

      float raw = readDistance();

      // DEBUG (uncomment if needed)
      // Serial.println(raw);

      if (raw > 0) {

        if (!kalmanInitialized) {
          kalman_x = raw;
          kalmanInitialized = true;
        }

        float filtered = kalmanFilter(raw);

        samples[validSamples++] = filtered;
      }
    }

    // -------- PROCESS --------
    if (validSamples >= 1) {

      float medianDistance = getMedian(samples, validSamples);

      float waterLevel = TANK_HEIGHT - (medianDistance - SENSOR_OFFSET);

      if (waterLevel < 0) waterLevel = 0;
      if (waterLevel > TANK_HEIGHT) waterLevel = TANK_HEIGHT;

      int percentInt = (int)(waterLevel + 0.5);

      // -------- TREND --------
      bool isRising = false;

      if (lastPercent >= 0) {
        isRising = (percentInt - lastPercent) >= 2;
      }

      if (abs(percentInt - lastPercent) >= 1) {
        lastPercent = percentInt;
      }

      // -------- PRINT RATE --------
      unsigned long printInterval = isRising ? 250 : 1000;

      if (now - lastPrintTime >= printInterval) {
        lastPrintTime = now;

        uint8_t data[1] = { (uint8_t)percentInt };
        uint8_t checksum = calculateChecksum(data, 1);

        Serial.print("Level: ");
        Serial.print(percentInt);
        Serial.print("% | ");

        Serial.print(isRising ? "RISING" : "STABLE");
        Serial.print(" | Checksum: ");
        Serial.println(checksum);
      }

    } else {
      // only show occasionally (NO SPAM)
      if (now - lastFailPrint > 1000) {
        lastFailPrint = now;
        Serial.println("Sensor waiting...");
      }
    }
  }
}