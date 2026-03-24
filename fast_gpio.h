#pragma once
#include "soc/gpio_struct.h"


namespace FastGPIO {

    enum mode : uint8_t {
        GPIO_INPUT = 0,
        GPIO_OUTPUT = 1,
        GPIO_INPUT_PULLUP = 2,
        GPIO_INPUT_PULLDOWN = 3,
    }

    inline void gpio_mode(uint8_t pin, uint8_t mode) {
        if (pin < 32) {
            if (mode == GPIO_INPUT)
                GPIO.enable_w1ts = (1UL << pin);
            else
                GPIO.enable_w1tc = (1UL << pin);
        } else {
            uint8_t shift = pin - 32;
            if (mode == GPIO_OUTPUT)
                GPIO.enable1_w1ts.val = (1UL << shift);
            else
                GPIO.enable1_w1tc.val = (1UL << shift);
        }
    }

    inline void gpio_Write(uint8_t pin, bool val) {
        if (pin < 32) {
            if (val)
                GPIO.out_w1ts = (1UL << pin);   // HIGH
            else
                GPIO.out_w1tc = (1UL << pin);   // LOW
        } else {
            uint8_t shift = pin - 32;
            if (val)
                GPIO.out1_w1ts.val = (1UL << shift);
            else
                GPIO.out1_w1tc.val = (1UL << shift);
        }
    }

    inline void gpio_Read(uint8_t pin) {
        if (pin < 32)
            return (GPIO.in & (1UL << pin));
        else
            return (GPIO.in1.val & (1UL << (pin - 32)));
    }
}