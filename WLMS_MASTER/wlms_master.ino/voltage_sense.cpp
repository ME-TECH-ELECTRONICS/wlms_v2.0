#include "voltage_sense.h"
#include "config.h"
#include "driver/adc.h"

int readADC(adc1_channel_t channel) {
    adc1_config_width(ADC_WIDTH_BIT_12);              // 0–4095
    adc1_config_channel_atten(channel, ADC_ATTEN_DB_11); // up to ~3.3V

    return adc1_get_raw(channel);
}

float readACVoltage(uint8_t pin, float sensitivity, uint16_t frequency = 50, uint8_t loops = 5) {
  uint32_t period = 1000000 / frequency;
  float totalVoltage = 0;

  for (uint8_t l = 0; l < loops; l++) {
    // Step 1: Find zero point (offset)
    uint32_t sum = 0;
    uint32_t count = 0;
    uint32_t start = micros();

    while (micros() - start < period) {
      sum += readADC(VOLTAGE_SENS);
      count++;
    }

    int zeroPoint = sum / count;

    // Step 2: Calculate RMS
    uint32_t sqSum = 0;
    count = 0;
    start = micros();

    while (micros() - start < period) {
      int val = readADC(VOLTAGE_SENS) - zeroPoint;
      sqSum += val * val;
      count++;
    }

    float rms = sqrt((float)sqSum / count);

    // Step 3: Convert to voltage
    float voltage = rms / 4095.0 * 3.3 * sensitivity;

    totalVoltage += voltage;
  }

  return totalVoltage / loops;
}

void voltageTask(void *pv) {
    while(1);
}