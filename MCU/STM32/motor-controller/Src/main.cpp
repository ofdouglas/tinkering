/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body (C++)
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"
#include "static_string.h"

#include "FreeRTOS.h"
#include "task.h"

namespace {

constexpr uint16_t kUartTaskStackWords = 512;
constexpr uint16_t kLedTaskStackWords = 256;
constexpr UBaseType_t kUartTaskPriority = tskIDLE_PRIORITY + 2;
constexpr UBaseType_t kLedTaskPriority = tskIDLE_PRIORITY + 1;

void UartTask(void *argument)
{
  (void)argument;

  static const StaticString kMsg = STATIC_STRING("Hello from STM32 motor controller v0.0.4!\n");

  for (;;)
  {
    (void)bsp.uartTransmit(reinterpret_cast<const uint8_t *>(kMsg.data),
                           static_cast<uint16_t>(kMsg.length), 10000U);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void LedTask(void *argument)
{
  (void)argument;

  for (;;)
  {
    bsp.ledToggle();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

} /* namespace */

int main(void)
{
  bsp.init();

  if (xTaskCreate(UartTask, "uart", kUartTaskStackWords, nullptr, kUartTaskPriority,
                  nullptr) != pdPASS)
  {
    ERROR("uart task create failed\n");
  }

  if (xTaskCreate(LedTask, "led", kLedTaskStackWords, nullptr, kLedTaskPriority,
                  nullptr) != pdPASS)
  {
    ERROR("led task create failed\n");
  }

  vTaskStartScheduler();

  ERROR("scheduler start failed\n");
}

extern "C" {

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  error_halt(reinterpret_cast<const char *>(file), line, "assert failed",
             sizeof("assert failed") - 1U);
}
#endif

} /* extern "C" */
