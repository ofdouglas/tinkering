/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "freertos/projdefs.h"
// #include "portmacro.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "server.h"

#define LED_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_21

/*
Second Task + Queue (events)
Button: debounce. ISR. generate events
Software timer callback
WiFi bring-up
TCP or UDP experiment
I2C sensor
*/

EventGroupHandle_t events_handle;
int led_on_event_bit = 1;
int led_off_event_bit = 2;

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
    (void) context;

    led_init();
    ESP_LOGI("ledTask", "LED initialized");

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(events_handle, 
                                    led_on_event_bit | led_off_event_bit, 
                                    pdFALSE, 
                                    pdFALSE, 
                                    portMAX_DELAY);
        if (bits & led_on_event_bit) {
            gpio_set_level(LED_PIN, 1);
            ESP_LOGI("ledTask", "Turn LED ON");
            xEventGroupClearBits(events_handle, led_on_event_bit);
        } else if (bits & led_off_event_bit) {
            gpio_set_level(LED_PIN, 0);
            ESP_LOGI("ledTask", "Turn LED OFF");
            xEventGroupClearBits(events_handle, led_off_event_bit);
        } else {
            ESP_LOGE("ledTask", "Event bit error");
        }
    }
}

void button_task(void* context) {
    (void) context;

    button_init();
    ESP_LOGI("buttonTask", "button initialized");

    bool button_on = false;
    bool led_on = false;
    uint8_t shift_reg = 0;
    const uint8_t shift_reg_mask = 0x1F;

    while (1) {
        // Pin has pullup; button press grounds the pin
        shift_reg = (shift_reg << 1) | (gpio_get_level(BUTTON_PIN) ? 0 : 1);
        const uint8_t shift_reg_masked = shift_reg & shift_reg_mask;

        if (!button_on && (shift_reg_masked == shift_reg_mask)) {
            button_on = true;
            led_on = !led_on;
            xEventGroupSetBits(events_handle, led_on ? led_on_event_bit : led_off_event_bit);
            xEventGroupClearBits(events_handle, led_on ? led_off_event_bit : led_on_event_bit);
            ESP_LOGI("buttonTask", "Button ON");
        } else if (button_on && (shift_reg_masked == 0x00)) {
            button_on = false;
            ESP_LOGI("buttonTask", "Button OFF");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void process_cmd(const char* cmd) {
    if (!strcmp("on", cmd)) {
        xEventGroupSetBits(events_handle, led_on_event_bit);
        xEventGroupClearBits(events_handle, led_off_event_bit);
        ESP_LOGI("main", "LED ON event");
    } else if (!strcmp("off", cmd)) {
        xEventGroupSetBits(events_handle, led_off_event_bit);
        xEventGroupClearBits(events_handle, led_on_event_bit);
        ESP_LOGI("main", "LED OFF event");
    }
}

void init() {
    events_handle = xEventGroupCreate();
    if (events_handle == NULL) {
        ESP_LOGE("main", "Event group creation failed");
        exit(1);
    }

    xTaskCreate(led_task, 
                "ledTask", 
                2048, 
                NULL, 
                1, 
                NULL);
    xTaskCreate(button_task, 
                "buttonTask", 
                2048, 
                NULL, 
                1, 
                NULL);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
        /* If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
         * and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. */
        esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    }

    ESP_LOGI("main", "ESP_WIFI_MODE_STA");
    wifi_init_sta();
}

void app_main(void)
{
    init();
    ESP_LOGI("main", "Hello world!");

    const size_t buffer_size = 32;
    char buffer[buffer_size];

    while (1) {
        int count = 0;
        while (count < buffer_size - 1) {
            int c = getchar();

            if (c == EOF) {
                vTaskDelay(pdMS_TO_TICKS(10));
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
