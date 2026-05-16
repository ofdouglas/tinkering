/**
  ******************************************************************************
  * @file    bsp.h
  * @brief   Board support package (STM32F746 Discovery)
  ******************************************************************************
  */

#ifndef BSP_H
#define BSP_H

#include "main.h"

/* On-board indicator: GPIOI pin 3 (Arduino D7 on Discovery header) */
#define BSP_LED_GPIO_Port GPIOI
#define BSP_LED_Pin GPIO_PIN_3

class Bsp
{
public:
  UART_HandleTypeDef huart1{};

  GPIO_TypeDef *led_gpio = BSP_LED_GPIO_Port;
  uint16_t led_pin = BSP_LED_Pin;

  /** MPU, HAL_Init, clocks, GPIO, USART1. */
  void init();

  void ledToggle();
  HAL_StatusTypeDef uartTransmit(const uint8_t *data, uint16_t len, uint32_t timeout);

private:
  void configureMpu();
  void configureSystemClock();
  void configureGpio();
  void configureUsart1();
};

extern Bsp bsp;

#endif /* BSP_H */
