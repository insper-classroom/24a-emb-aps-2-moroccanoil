/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/pwm.h"
#include "hardware/adc.h"

// a fila para os valores rgb precisa de uma struct (pq eh um valor que contem 3 valores)
typedef struct rgb {
    int r;
    int g;
    int b;
} rgb_t;

QueueHandle_t xQueueBTN; //envio e recebimento dos dados lidos do potenciometro
QueueHandle_t xQueueRGB; //envio e recebimento do valor RGB

#define PWM_0_GP 11
#define PWM_1_GP 12
#define PWM_2_GP 13
#define ADC_GP 26
#define ADC_MUX_ID 0

void wheel(uint WheelPos, uint8_t *r, uint8_t *g, uint8_t *b);

//esse timer callback fica realizando a leitura dos valores que chegam nesse pino 
bool timer_0_callback(repeating_timer_t *rt) {
    adc_select_input(ADC_MUX_ID);
    uint16_t result = adc_read();

    xQueueSendFromISR(xQueueBTN, &result, 0);
    return true; // keep repeating

    //USAR TIMER OU CHAMAR UM CALLBACK TODA VEZ QUE OS BTNS FOREM PRESSIONADOS?
}

// recebe os valores da de leitura do potenciometro atraves da fila ADC
// configura o timer pra gerar 5 IRS por segundo
// chama a funcao wheel
// envia o valor RGB para led task
void adc_task(void *p) {
    adc_init();
    adc_gpio_init(ADC_GP);

    uint16_t adc_value;
    printf("oi \n");
    while (true) {
        if (xQueueReceive(xQueueBTN, &adc_value, pdMS_TO_TICKS(100))) {
            //se green=1: on
            //se red=1: lavaboy
            //se blue=1: watergirl
            uint8_t r=0, g=0, b=0;
            wheel(adc_value, &r, &g, &b);
            rgb_t rgb = {r, g, b};

            xQueueSend(xQueueRGB, &rgb, 0);
        }
    }
}

// recebe os dados da fila RGB
void led_task(void *p) {
    rgb_t rgb;
    while (true) {

        if (xQueueReceive(xQueueRGB, &rgb, pdMS_TO_TICKS(100))) {
            printf("r: %d, g: %d, b: %d\n", rgb.r, rgb.g, rgb.b);
            //LIGAR LED - FAZER POR FILA OU DIRETO NO CALLBACK DO BTN?
        }
    }
}

// esta definida em outro lugar, conversao de valor analogico em rgb
// os valores recebidos no pino ADC sao lidos pelo callback, mandados para 
// a o adc_task atraves da QueueADC, onde eh chamada funcao wheel que converte em valores tipo RGB,
// que sao enviados para o led_task atraves da QueueRGB
void wheel(uint WheelPos, uint8_t *r, uint8_t *g, uint8_t *b) {
    WheelPos = 255 - WheelPos;

    if (WheelPos < 85) {
        *r = 255 - WheelPos * 3;
        *g = 0;
        *b = WheelPos * 3;
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        *r = 0;
        *g = WheelPos * 3;
        *b = 255 - WheelPos * 3;
    } else {
        WheelPos -= 170;
        *r = WheelPos * 3;
        *g = 255 - WheelPos * 3;
        *b = 0;
    }
}

int main() {
    stdio_init_all();
    printf("oi\n");
    xQueueBTN = xQueueCreate(32, sizeof(uint16_t));
    xQueueRGB = xQueueCreate(32, sizeof(rgb_t));

    xTaskCreate(adc_task, "adc", 4095, NULL, 1, NULL);
    xTaskCreate(led_task, "led", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}