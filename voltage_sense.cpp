#include <Arduino.h>
#include "esp_adc/adc_continuous.h"
#include <math.h>

// -------- CONFIG --------
#define ADC_CHANNEL ADC_CHANNEL_5  // GPIO33
#define SAMPLE_RATE 20000
#define BUFFER_SIZE 256
#define RING_SIZE 4096
#define MAINS_FREQ 50
#define CALIBRATION_FACTOR 0.581f

adc_continuous_handle_t adc_handle;

// -------- Ring Buffer --------
volatile int ring[RING_SIZE];
volatile uint16_t head = 0;
volatile uint16_t tail = 0;

static float zeroPoint = 2048;
static float filtered = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

inline bool ringPush(int v) {
  portENTER_CRITICAL(&mux);

  uint16_t next = (head + 1) % RING_SIZE;
  if (next == tail) {
    portEXIT_CRITICAL(&mux);
    return false;
  }

  ring[head] = v;
  head = next;

  portEXIT_CRITICAL(&mux);
  return true;
}

inline bool ringPop(int &v) {
  portENTER_CRITICAL(&mux);

  if (head == tail) {
    portEXIT_CRITICAL(&mux);
    return false;
  }

  v = ring[tail];
  tail = (tail + 1) % RING_SIZE;

  portEXIT_CRITICAL(&mux);
  return true;
}

void initADC() {
  esp_err_t err;

  adc_continuous_handle_cfg_t handle_config = {
    .max_store_buf_size = 2048,
    .conv_frame_size = BUFFER_SIZE,
  };
  err = adc_continuous_new_handle(&handle_config, &adc_handle);
  if (err != ESP_OK) {
    Serial.println("ADC handle init failed");
    while (1)
      ;
  }


  adc_continuous_config_t adc_config = {
    .sample_freq_hz = SAMPLE_RATE,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
  };

  adc_digi_pattern_config_t pattern = {
    .atten = ADC_ATTEN_DB_12,
    .channel = ADC_CHANNEL,
    .unit = ADC_UNIT_1,
    .bit_width = ADC_BITWIDTH_12,
  };

  adc_config.pattern_num = 1;
  adc_config.adc_pattern = &pattern;

  err = adc_continuous_config(adc_handle, &adc_config);
  if (err != ESP_OK) {
    Serial.println("ADC config failed");
    while (1)
      ;
  }
  err = adc_continuous_start(adc_handle);
  if (err != ESP_OK) {
    Serial.println("ADC start failed");
    while (1)
      ;
  }
}

void readADC(void *pv) {

  uint8_t buffer[BUFFER_SIZE];
  uint32_t length = 0;

  while (1) {

    if (adc_continuous_read(adc_handle, buffer, BUFFER_SIZE, &length, 1000) == ESP_OK) {

      if (length < sizeof(adc_digi_output_data_t)) continue;

      int samples = length / sizeof(adc_digi_output_data_t);

      adc_digi_output_data_t *p = (adc_digi_output_data_t *)buffer;

      for (int i = 0; i < samples; i++) {

        uint16_t raw = p[i].type1.data & 0x0FFF;

        ringPush(raw);
      }
    }
  }
}

void readVoltageTask(void *pv) {

  uint64_t sqSum = 0;
  uint32_t count = 0;

  int requiredSamples = (SAMPLE_RATE / MAINS_FREQ) * 10;  // 10 cycles

  while (1) {

    int raw;

    if (ringPop(raw)) {

      // Offset tracking
      zeroPoint = 0.999f * zeroPoint + 0.001f * raw;

      int val = raw - (int)zeroPoint;

      sqSum += (uint64_t)val * val;
      count++;

      if (count >= requiredSamples) {

        float rms = sqrt((float)sqSum / count);
        float v = rms * CALIBRATION_FACTOR;

        if (filtered == 0) filtered = v;
        filtered = 0.8f * filtered + 0.2f * v;

        int voltage = (int)(filtered + 0.5f);

        Serial.print("Voltage: ");
        Serial.println(voltage);

        sqSum = 0;
        count = 0;
      }

    } else {
      vTaskDelay(1);  // no data → yield CPU
    }
  }
}