/**
  ******************************************************************************
  * @file    hal_msp_pins.h
  * @brief   Pin macros referenced by stm32f7xx_hal_msp.c only
  ******************************************************************************
  */

#ifndef HAL_MSP_PINS_H
#define HAL_MSP_PINS_H

/* USART1 (ST-Link VCP) */
#define VCP_TX_Pin GPIO_PIN_9
#define VCP_TX_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_7
#define VCP_RX_GPIO_Port GPIOB

#endif /* HAL_MSP_PINS_H */
