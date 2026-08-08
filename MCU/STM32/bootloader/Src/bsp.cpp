/**
  ******************************************************************************
  * @file    bsp.cpp
  * @brief   Board support package implementation
  ******************************************************************************
  */

#include "bsp.h"
#include "bsp/base_bsp.h"
#include "hal/clock.h"

#include "stm32f746xx.h"

///////////////////////////////////////////////////////////////////////////////
// Global BSP instance
///////////////////////////////////////////////////////////////////////////////
Bsp bsp{};


///////////////////////////////////////////////////////////////////////////////
// Bsp::UartLogSink implementation
///////////////////////////////////////////////////////////////////////////////
Bsp::UartLogSink::UartLogSink(UART_HandleTypeDef &huart)
    : huart_(huart) {}

Bsp::UartLogSink::~UartLogSink() {}

// TODO: buffer any writes which occur before the UART is initialized
bool Bsp::UartLogSink::write(util::Span<const uint8_t> message) {
    return HAL_UART_Transmit(&huart_, const_cast<uint8_t*>(message.data()), static_cast<uint16_t>(message.size()), 1000) == HAL_OK;
}


///////////////////////////////////////////////////////////////////////////////
// SysTick Scheduler clock implementation
///////////////////////////////////////////////////////////////////////////////
volatile uint32_t sys_tick_count = 0U;

hal::SchedulerClock::TimePoint hal::SchedulerClock::now() noexcept {
    return hal::SchedulerClock::TimePoint(std::chrono::milliseconds(sys_tick_count));
}

hal::PlatformClock::TimePoint hal::PlatformClock::now() noexcept {
    return hal::PlatformClock::TimePoint(
        std::chrono::duration_cast<hal::PlatformClock::Duration>(std::chrono::milliseconds(sys_tick_count)));
}

extern "C" {
    void SysTick_Handler(void) {
        sys_tick_count++;
    }
}

void Bsp::configureSysTick() {
    SysTick->LOAD = (Bsp::kSystemCoreClockHz / Bsp::kSysTickFrequencyHz) - 1U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}


///////////////////////////////////////////////////////////////////////////////
// Bsp class implementation
///////////////////////////////////////////////////////////////////////////////
bool Bsp::earlyInit() noexcept {
    // TODO: buffer any writes which occur before the UART is initialized
    logging::setLogSink(&uart_log_sink_);

    HAL_Init();
    configureSystemClock();
    configureGpio();
    configureUsart1();
    configureUsart6();
    setDebugLed(false);

    configureSysTick();
    __enable_irq();

    return true;
}

void Bsp::configureSystemClock() {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 400;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        LOG_FATAL() << "RCC osc config failed";
    }
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        LOG_FATAL() << "PWR over-drive failed";
    }
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK) {
        LOG_FATAL() << "RCC clock config failed";
    }
}

void Bsp::configureUsart1() {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        LOG_FATAL() << "USART1 init failed";
    }

    NVIC_SetPriority(USART1_IRQn, kUart1IrqPriority);
    NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

void Bsp::configureUsart6() {
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        LOG_FATAL() << "USART6 init failed";
    }
}

void Bsp::configureGpio() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOI_CLK_ENABLE();
    HAL_GPIO_WritePin(led_gpio, led_pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = led_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(led_gpio, &GPIO_InitStruct);
}

void Bsp::setDebugLed(bool on) noexcept {
    led_on_ = on;
    HAL_GPIO_WritePin(led_gpio, led_pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Bsp::toggleDebugLed() noexcept {
    setDebugLed(led_on_);
    led_on_ = !led_on_;
}

void Bsp::SystemReset::reset() noexcept {
    NVIC_SystemReset();
}
