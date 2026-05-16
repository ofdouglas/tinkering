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

/* USART6 (Arduino header) */
#define ARDUINO_RX_D0_Pin GPIO_PIN_7
#define ARDUINO_RX_D0_GPIO_Port GPIOC
#define ARDUINO_TX_D1_Pin GPIO_PIN_6
#define ARDUINO_TX_D1_GPIO_Port GPIOC

/* ADC3 (Arduino analog) */
#define ARDUINO_A0_Pin GPIO_PIN_0
#define ARDUINO_A0_GPIO_Port GPIOA
#define ARDUINO_A1_Pin GPIO_PIN_10
#define ARDUINO_A1_GPIO_Port GPIOF
#define ARDUINO_A2_Pin GPIO_PIN_9
#define ARDUINO_A2_GPIO_Port GPIOF
#define ARDUINO_A3_Pin GPIO_PIN_8
#define ARDUINO_A3_GPIO_Port GPIOF
#define ARDUINO_A4_Pin GPIO_PIN_7
#define ARDUINO_A4_GPIO_Port GPIOF
#define ARDUINO_A5_Pin GPIO_PIN_6
#define ARDUINO_A5_GPIO_Port GPIOF

/* TIM PWM (Arduino pins) */
#define ARDUINO_PWM_D10_Pin GPIO_PIN_8
#define ARDUINO_PWM_D10_GPIO_Port GPIOA
#define ARDUINO_PWM_D9_Pin GPIO_PIN_15
#define ARDUINO_PWM_D9_GPIO_Port GPIOA
#define ARDUINO_PWM_D3_Pin GPIO_PIN_4
#define ARDUINO_PWM_D3_GPIO_Port GPIOB
#define ARDUINO_PWM_CS_D5_Pin GPIO_PIN_0
#define ARDUINO_PWM_CS_D5_GPIO_Port GPIOI
#define ARDUINO_PWM_D6_Pin GPIO_PIN_6
#define ARDUINO_PWM_D6_GPIO_Port GPIOH

#endif /* HAL_MSP_PINS_H */
