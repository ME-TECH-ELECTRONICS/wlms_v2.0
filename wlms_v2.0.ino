#include <Arduino.h>
#include <soc/gpio_struct.h>
#include <driver/gpio.h>

#define LED_PIN 2

void setup()
{
  // Serial.begin(115200);

  // Set GPIO4 as OUTPUT
  GPIO.enable_w1ts = (1 << LED_PIN);
}

void loop()
{
  // Set HIGH
  GPIO.out_w1ts = (1 << LED_PIN);
  delay(500);

  // Set LOW
  GPIO.out_w1tc = (1 << LED_PIN);
  delay(500);

  pinMode()
}