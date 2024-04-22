
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

// const int HC06_UART_ID = 1;
// const int HC06_BAUD_RATE = 9600;

// const int HC06_TX_PIN = 4;
// const int HC06_RX_PIN = 5;

const int BUTTON_JUMP_RED_PIN = 21;
const int BUTTON_SWITCH_BLUE_PIN = 20;

const int LED_RED_PIN = 7;
const int LED_BLUE_PIN = 6;
const int LED_GREEN_PIN = 22;

const int ADC0_X = 26; //x
const int ADC0_CHANNEL = 0;
// const int ADC1_Y = 27; //y
// const int ADC1_CHANNEL = 1;

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

// void btn_callback(){
//     if (gpio_get(BUTTON_JUMP_RED_PIN) == 1){
//         int btn_red = 1;
//         xQueueSend(xQueueBtn, &btn_red, portMAX_DELAY);
//     }
//     if (gpio_get(BUTTON_SWITCH_BLUE_PIN) == 1){
//         int btn_axis = 0;
//         xQueueSend(xQueueBtn, &btn_axis, portMAX_DELAY);
//     }
//     else if (gpio_get(BUTTON_SWITCH_BLUE_PIN) == 0){
//         int btn_axis = 1;
//         xQueueSend(xQueueBtn, &btn_axis, portMAX_DELAY);
//     }
// }

void btn_callback(){
    adc_t data;
    if (gpio_get(BUTTON_JUMP_RED_PIN) == 1){
        btn_value = 3;
    }
    if (gpio_get(BUTTON_JUMP_RED_PIN) == 0){
        btn_value = 0;
    }
    if (gpio_get(BUTTON_SWITCH_BLUE_PIN) == 1){
        btn_axis = !btn_axis;
    }
    // else if (gpio_get(BUTTON_SWITCH_BLUE_PIN) == 0){
    //     data.axis = 1;
    //     btn_axis = 1;
    //     xQueueSend(xQueueAdc, &data, portMAX_DELAY);
    // }
}

// void write_package(adc_t data, int person_id) {
//     if (data.val)
//     int val = data.val;
//     // int msb = val >> 8;
//     // int lsb = val & 0xFF ;

//     uart_putc_raw(uart0, data.axis); 
//     // uart_putc_raw(uart0, lsb);
//     // uart_putc_raw(uart0, msb); 
//     uart_putc_raw(uart0, -1); 
// }

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

void x_task(void *p) {
    adc_t data;if (gpio_get(BUTTON_JUMP_RED_PIN) == 0){
        data.val = 0;
        data.axis = btn_axis;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
    }
    if (gpio_get(BUTTON_SWITCH_BLUE_PIN) == 1){
        btn_axis = !btn_axis;
    adc_init();
    adc_gpio_init(ADC0_X);

    int v[5];
    //int mediaX;

    while (1) {
        adc_select_input(ADC0_CHANNEL);
        v[0]=v[1];
        v[1]=v[2];
        v[2]=v[3];
        v[3]=v[4];
        v[4]=adc_read();
        int mediaX = ((v[4]+v[3]+v[2]+v[1]+v[0])/5);

        int coisa = adjust_scale(adc_read());

        if(coisa > 100){ 
            data.val = 2;
            data.axis = btn_axis;
            xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        } else if (coisa < -100){
            data.val = 1;
            data.axis = btn_axis; 
            xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        } else {
            data.val = 0;
            data.axis = btn_axis;
            xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
}

void btn_task() {
    adc_t data;
    data.val = btn_value;
    data.axis = btn_axis;
    xQueueSend(xQueueAdc, &data, portMAX_DELAY);
}

void uart_task(void *p) {
    adc_t data;
    int btn_red = 0;
    // int btn_axis;
    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);

        // // recebe fila do botao na var btn_axis
        // // if (xQueueReceive(xQueueBtn, &btn_axis, 0) == pdTRUE) {
        // //     if (btn_axis == 1) {
        // //         btn_red = !btn_red;   
        // //     }
        // // }

        // //printf("%d\n", data);
        //if (data.val != 0) write_package(data, btn_red);
        if (data.val != 0) write_package(data);
    }
}



void hc06_task(void *p) {
    adc_read();
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("aps2_legal", "1234");

    while (true) {
        uart_puts(HC06_UART_ID, "OLAAA ");
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

    // printf("Start bluetooth task\n");BUTTON_SWITCH_BLUE_PIN;

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));
    xQueueBtn = xQueueCreate(32, sizeof(uint64_t));
    xTaskCreate(hc06_task, "UART_Task 1", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(btn_task, "btn_task", 4096, NULL, 1, NULL);
    
    
    vTaskStartScheduler();

    gpio_set_irq_enabled_with_callback(BUTTON_JUMP_RED_PIN, GPIO_IRQ_EDGE_FALL, true,
                                     &btn_callback);
                                    
    gpio_set_irq_enabled(BUTTON_SWITCH_BLUE_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true)
        ;
}
