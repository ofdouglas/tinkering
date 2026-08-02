/**
  ******************************************************************************
  * @file    bsp.h
  * @brief   Board support package (STM32F746 Discovery)
  ******************************************************************************
  */

#ifndef BSP_H
#define BSP_H

#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include "stm32f7xx_hal_gpio.h"

#include "bsp/base_bsp.h"
#include "logging/log.hpp"

/* On-board indicator: GPIOI pin 3 (Arduino D7 on Discovery header) */
#define BSP_LED_GPIO_Port GPIOI
#define BSP_LED_Pin GPIO_PIN_3


class Bsp : public BaseBsp {
public:
    static constexpr uint32_t kSystemCoreClockHz = 200'000'000U;
    static constexpr uint32_t kSysTickFrequencyHz = 1000U;

    // Data UART (ST-Link VCP)
    UART_HandleTypeDef huart1{};

    // Debug UART (Arduino D0/D1)
    UART_HandleTypeDef huart6{};

    GPIO_TypeDef* led_gpio = BSP_LED_GPIO_Port;
    uint16_t led_pin = BSP_LED_Pin;

    /** HAL_Init, clocks, GPIO, USART1. */
    bool earlyInit() noexcept override;

    void setDebugLed(bool on) noexcept override;

    void toggleDebugLed() noexcept override;

    // TODO: deconflict with log sink
    bool uartWrite(Span<const uint8_t> data);

    bool uartRead(Span<uint8_t> output, size_t rx_num);

private:
    static constexpr uint8_t kUart1IrqPriority{3U};

    class UartLogSink : public logging::LogSink {
    public:
        explicit UartLogSink(UART_HandleTypeDef &huart);
        ~UartLogSink() override;
        bool write(Span<const uint8_t> message) override;
    private:
        UART_HandleTypeDef &huart_;
    };

    void configureSystemClock();
    void configureSysTick();
    void configureGpio();
    void configureUsart6();
    void configureUsart1();

    UartLogSink uart_log_sink_{huart6};
    bool led_on_ = false;
};

extern Bsp bsp;



#endif /* BSP_H */
