/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "freertos/projdefs.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_21

/*
Second Task + Queue (events)
Button: debounc. ISR. generate events
Software timer callback
WiFi bring-up
TCP or UDP experiment
I2C sensor
*/



int g_led_enable = 1;

void led_init(void) {
    gpio_config_t config = {
        1 << LED_PIN,
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_DISABLE,
        GPIO_PULLDOWN_DISABLE,
        GPIO_INTR_DISABLE
    };
    gpio_config(&config);
}

void button_init(void) {
    gpio_config_t config = {
        1 << BUTTON_PIN,
        GPIO_MODE_INPUT,
        GPIO_PULLUP_ENABLE,
        GPIO_PULLDOWN_DISABLE,
        GPIO_INTR_DISABLE
    };
    gpio_config(&config);
}

void led_task(void* context) {
    (void)context;

    led_init();
    button_init();
    // printf("LED initialized.\n");
    ESP_LOGI("ledTask", "LED initialized.\n");

    int gpio_state = 0;
    while (1) {
        gpio_set_level(LED_PIN, gpio_state & g_led_enable);
        gpio_state ^= 1;

        int ticks_to_delay = gpio_get_level(BUTTON_PIN) ? 500 : 250;
        vTaskDelay(ticks_to_delay);
    }
}


void process_cmd(const char* cmd) {
    if (!strcmp("off", cmd)) {
        g_led_enable = 0;
    } else if (!strcmp("on", cmd)) {
        g_led_enable = 1;
    }
}

void app_main(void)
{
    ESP_LOGI("main", "Hello world!\n");

    int val = xTaskCreate(led_task, 
                "ledTask", 
                2048, 
                NULL, 
                1, 
                NULL);

    ESP_LOGI("main", "xTaskCreate returned %d\n", val);


    const size_t buffer_size = 32;
    char buffer[buffer_size];

    while (1) {
        int count = 0;
        while (count < buffer_size - 1) {
            int c = getchar();

            if (c == EOF) {
                vTaskDelay(pdMS_TO_TICKS(10))
                continue;
            } 
            putchar(c);
            fflush(stdin);

            if (c == '\n') {
                buffer[count] = '\0';
                break;
            } else {
                buffer[count++] = c;
            }
        }

        printf("Got string: %s\n", buffer);
        process_cmd(buffer);
    }
}
