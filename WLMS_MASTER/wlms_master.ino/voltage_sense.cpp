#include "voltage_sense.h"
#include "config.h"
#include "esp_adc/adc_oneshot.h"
#include <math.h>

adc_oneshot_unit_handle_t adc_handle;

// -------- CONFIG --------
#define VOLTAGE_SENS ADC1_CHANNEL_6  // change to your pin
#define SAMPLE_RATE 2000             // Hz
#define MAINS_FREQ 50                // Hz
#define CALIBRATION_FACTOR 126.0f    // <-- YOU MUST CALIBRATE THIS
static float zeroPoint = 2048;       // start mid-scale

void initADC() {
  adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = ADC_UNIT_1,
  };
  adc_oneshot_new_unit(&init_config, &adc_handle);

  adc_oneshot_chan_cfg_t config = {
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten = ADC_ATTEN_DB_11,
  };

  adc_oneshot_config_channel(adc_handle, ADC1_CHANNEL_6, &config);
}

int readADC(adc_channel_t channel) {
  int raw = 0;
  adc_oneshot_read(adc_handle, channel, &raw);
  return raw;
}

// -------- MAIN FUNCTION --------
float readACVoltage() {

  const uint32_t period_us = 1000000 / MAINS_FREQ;
  const uint32_t sampleInterval = 1000000 / SAMPLE_RATE;

  uint32_t start = micros();
  uint32_t lastSample = micros();

  uint64_t sqSum = 0;  // FIX: prevent overflow
  uint32_t count = 0;

  while (micros() - start < period_us) {

    if (micros() - lastSample >= sampleInterval) {
      lastSample += sampleInterval;

      int raw = readADC(VOLTAGE_SENS);

      // -------- OFFSET FILTER (slow tracking) --------
      zeroPoint = 0.995f * zeroPoint + 0.005f * raw;

      int val = raw - (int)zeroPoint;

      sqSum += (uint64_t)val * val;
      count++;
    }
  }

  if (count == 0) return 0;

  float rms = sqrt((float)sqSum / count);

  // -------- FINAL VOLTAGE --------
  float voltage = rms * CALIBRATION_FACTOR;

  return voltage;
}

void voltageTask(void *pv) {
  while (1)
    ;
}