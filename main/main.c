#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <string.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"
#include "hc06.h"

#include <math.h>
#include <stdlib.h>

const int BUTTON_JUMP_RED_PIN = 21;
const int BUTTON_SWITCH_BLUE_PIN = 16;

const int LED_RED_PIN = 7;
const int LED_BLUE_PIN = 6;
const int LED_GREEN_PIN = 22;

const int ADC0_X = 26; //x
const int ADC0_CHANNEL = 0;

int volatile btn_axis = 0;
int volatile btn_value = 0;

QueueHandle_t xQueueAdc;
QueueHandle_t xQueueBtn;

typedef struct adc {
    int axis;
    int val;
} adc_t;

int adjust_scale(int adc_value) {
    // Mapear de 0-4095 para -255 a +255
    int adjusted_value = ((adc_value - 2048) * 255) / 2048;

    // Aplicar a zona morta
    if (adjusted_value > -200 && adjusted_value < 200) {
        return 0;
    } else {
        return adjusted_value;
    }
}

void btn_callback(uint gpio, uint32_t events) {
    adc_t data;
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (gpio == BUTTON_JUMP_RED_PIN) {
            data.val = 3;
            data.axis = btn_axis;
        }

        if (gpio == BUTTON_SWITCH_BLUE_PIN) {
            btn_axis = !btn_axis;
            data.val = 0;
            }
        
    }

    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio == BUTTON_JUMP_RED_PIN) {
            data.val = 0;
            data.axis = btn_axis;
        }
    }

    xQueueSend(xQueueAdc, &data, portMAX_DELAY);
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart1, data.axis);
    uart_putc_raw(uart1, lsb);
    uart_putc_raw(uart1, msb);
    uart_putc_raw(uart1, -1);

    // uart_putc_raw(uart0, data.axis);
    // uart_putc_raw(uart0, lsb);
    // uart_putc_raw(uart0, msb);
    // uart_putc_raw(uart0, -1);

 }

void x_task(void *p) {
    adc_t data;

    adc_init();
    adc_gpio_init(ADC0_X);

    int last_value = 0;
    xQueueSend(xQueueAdc, &data, portMAX_DELAY);
    

    int v[5];

    while (1) {
        adc_select_input(ADC0_CHANNEL);
        v[0] = v[1];
        v[1] = v[2];
        v[2] = v[3];
        v[3] = v[4];
        v[4] = adc_read();
        int mediaX = ((v[4] + v[3] + v[2] + v[1] + v[0]) / 5);

        int coisa = adjust_scale(adc_read());

        if (coisa > 220) {
            data.val = 2;
            data.axis = btn_axis;
            
                xQueueSend(xQueueAdc, &data, portMAX_DELAY);
                last_value = data.val;
        }
            
        else if (coisa < -220) {
            data.val = 1;
            data.axis = btn_axis;
            
                xQueueSend(xQueueAdc, &data, portMAX_DELAY);
             
        }
        else {
            data.val = 0;
            data.axis = btn_axis;
            
                xQueueSend(xQueueAdc, &data, portMAX_DELAY);
                
            
        }

        vTaskDelay(pdMS_TO_TICKS(50));

    }
}


void led_task(void *p) {

    while (1) {
        
            if (btn_axis == 0) {
                // Acende led azul
                gpio_put(LED_BLUE_PIN, 1);
                gpio_put(LED_RED_PIN, 0);
            }

            if (btn_axis == 1) {
                // Acende led vermelho
                gpio_put(LED_RED_PIN, 1);
                gpio_put(LED_BLUE_PIN, 0);
            }
        }
    
}

void uart_task(void *p) {
    static int uart_counter = 0; // Contador para controlar chamadas da uart_task
    adc_t data;
    int btn_red = 0;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY) == pdTRUE) {
            if (data.axis == 0 || data.axis == 1) {
                write_package(data);
            }
        

            
            }
        }
    }


void hc06_task(void *p) {
    adc_read();
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("eu_tentei", "1234");
    // pisca led verde
    gpio_put(LED_GREEN_PIN, 1);
    gpio_put(LED_GREEN_PIN, 0);
    gpio_put(LED_GREEN_PIN, 1);
    gpio_put(LED_GREEN_PIN, 0);
    gpio_put(LED_GREEN_PIN, 1);
    gpio_put(LED_GREEN_PIN, 0);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_RED_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(BUTTON_JUMP_RED_PIN);
    gpio_init(BUTTON_SWITCH_BLUE_PIN);

    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_JUMP_RED_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_SWITCH_BLUE_PIN, GPIO_IN);

    gpio_pull_up(BUTTON_JUMP_RED_PIN);
    gpio_pull_up(BUTTON_SWITCH_BLUE_PIN);

    gpio_set_irq_enabled_with_callback(BUTTON_JUMP_RED_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled(BUTTON_SWITCH_BLUE_PIN, GPIO_IRQ_EDGE_FALL, true);

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));
    xQueueBtn = xQueueCreate(32, sizeof(uint64_t));
    xTaskCreate(hc06_task, "UART_Task 1", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    // Adicionando a criação da led_task aqui
    xTaskCreate(led_task, "led_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
