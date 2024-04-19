//MORSE lab 2

/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int btn_yellow = 15;
const int btn_black = 14;
const int buzzer = 18;

volatile bool line = false;
volatile bool dot = false;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == btn_yellow) {
        line = true;
    }
    if (gpio == btn_black) {
        dot = true;
    }
}

int main() {
    stdio_init_all();

    gpio_init(btn_yellow);
    gpio_set_dir(btn_yellow, GPIO_IN);
    gpio_pull_up(btn_yellow);

    gpio_init(btn_black);
    gpio_set_dir(btn_black, GPIO_IN);
    gpio_pull_up(btn_black);


    gpio_set_irq_enabled_with_callback(btn_black, GPIO_IRQ_EDGE_FALL, true,
                                     &btn_callback);
                                    
    gpio_set_irq_enabled(btn_yellow, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        if (line) {
            sleep_us(10000);
            buzzer_sound(6000, 300, buzzer);
            line = false;
        }
        if (dot) {
            sleep_us(100000);
            buzzer_sound(1000, 100, buzzer);
            dot = false;
        }
    } 
}