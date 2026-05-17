/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body (C++)
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"
#include "log.hpp"

#include "FreeRTOS.h"
#include "task.h"

using namespace literals;

namespace {

constexpr uint16_t kUartTaskStackWords = 512;
constexpr uint16_t kLedTaskStackWords = 256;
constexpr UBaseType_t kUartTaskPriority = tskIDLE_PRIORITY + 2;
constexpr UBaseType_t kLedTaskPriority = tskIDLE_PRIORITY + 1;

void UartTask(void *argument) {
    (void)argument;

    for (;;) {
        logging::info("Hello from STM32 motor controller version"_lit, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void LedTask(void *argument) {
    (void)argument;

    for (;;) {
        bsp.ledToggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

} /* namespace */

int main(void) {
    bsp.init();

    if (xTaskCreate(UartTask, "uart", kUartTaskStackWords, nullptr, kUartTaskPriority,
                    nullptr) != pdPASS) {
        logging::fatal("uart task create failed\n"_lit);
    }

    if (xTaskCreate(LedTask, "led", kLedTaskStackWords, nullptr, kLedTaskPriority,
                    nullptr) != pdPASS) {
        logging::fatal("led task create failed\n"_lit);
    }

    vTaskStartScheduler();

    logging::fatal("scheduler start failed\n"_lit);
}

extern "C" {

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    log_fatal_c(reinterpret_cast<const char *>(file), line, "assert failed",
                sizeof("assert failed") - 1U);
}
#endif

} /* extern "C" */
