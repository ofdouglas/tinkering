/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body (C++)
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"
#include "log.hpp"

#include "deferred_logger.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include <cstdint>

constexpr uint16_t kUartTaskStackWords = 512;
constexpr uint16_t kLedTaskStackWords = 256;
constexpr UBaseType_t kUartTaskPriority = tskIDLE_PRIORITY + 2;
constexpr UBaseType_t kLedTaskPriority = tskIDLE_PRIORITY + 1;

void UartTask(void* argument) {
    (void)argument;

    {
        DEFERRED_LOG_INFO() << "Deferred log test: x=" << (uint32_t)1U << ", y=" << (int32_t)2U;
    }

    for (;;) {
        LOG_INFO() << "Uptime: "_l << HAL_GetTick() << " ms"_l;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void LedTask(void* argument) {
    (void)argument;

    for (;;) {
        bsp.ledToggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void) {
    bsp.init();

    LOG_INFO() << "BSP initialized"_l;

    uint32_t* p = reinterpret_cast<uint32_t*>(0xDEADBEEFU);
    LOG_INFO() << "p: " << p;

    if (xTaskCreate(UartTask, "uart", kUartTaskStackWords, nullptr, kUartTaskPriority,
                    nullptr) != pdPASS) {
        LOG_FATAL() << "uart task create failed"_l;
    }

    if (xTaskCreate(LedTask, "led", kLedTaskStackWords, nullptr, kLedTaskPriority,
                    nullptr) != pdPASS) {
        LOG_FATAL() << "led task create failed"_l;
    }

    LOG_INFO() << "Starting scheduler: "_l << true;

    vTaskStartScheduler();

    LOG_FATAL() << "scheduler start failed"_l;
    return 1;
}

extern "C" {

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
    log_fatal_c(reinterpret_cast<const char*>(file), line, "assert failed",
                sizeof("assert failed") - 1U);
}
#endif

}
