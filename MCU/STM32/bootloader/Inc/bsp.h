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
#include "hal/reset_interface.h"
#include "interfaces/memory_interface.h"
#include "data_structures/span.h"

#include <array>

/* On-board indicator: GPIOI pin 3 (Arduino D7 on Discovery header) */
#define BSP_LED_GPIO_Port GPIOI
#define BSP_LED_Pin GPIO_PIN_3

extern "C" {
extern uint32_t _APP_FLASH_START;
extern uint32_t _APP_FLASH_SIZE;
}

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

    class SystemReset : public hal::ResetInterface {
    public:
        void reset() noexcept override;
    };

    SystemReset system_reset{};

    static constexpr uint32_t kAppRegionAttributes =
        static_cast<uint32_t>(Memory::Region::Attributes::kReadable) |
        static_cast<uint32_t>(Memory::Region::Attributes::kWriteable) |
        static_cast<uint32_t>(Memory::Region::Attributes::kBootable) |
        static_cast<uint32_t>(Memory::Region::Attributes::kIsFlash);

    std::array<Memory::Region, 1U> memory_region_table_{{
        {_APP_FLASH_START, _APP_FLASH_SIZE, kAppRegionAttributes, nullptr}
    }};
    Span<const Memory::Region> memory_regions{
        memory_region_table_.data(),
        memory_region_table_.size()
    };

    /** HAL_Init, clocks, GPIO, USART1, USART6. */
    bool earlyInit() noexcept override;

    void setDebugLed(bool on) noexcept override;

    void toggleDebugLed() noexcept override;

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
