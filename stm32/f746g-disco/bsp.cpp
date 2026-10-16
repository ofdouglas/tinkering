#include "bsp.h"

#include <atomic>
#include <chrono>
#include <cstdint>

#include "stm32f746xx.h"

#include "hal/clock.h"

namespace {

std::atomic<uint32_t> sys_tick_count{};

}  // namespace

extern "C" void SysTick_Handler() {
    sys_tick_count++;
}

hal::SchedulerClock::TimePoint hal::SchedulerClock::now() noexcept {
    return hal::SchedulerClock::TimePoint(std::chrono::milliseconds(sys_tick_count));
}

void Bsp::init() noexcept {
    reset_manager_.readResetCause();

    initCpu();
    initClocks();
    initSysTick();
    initGpio();

    uart_.init();
    // watchdog_.init(); TODO: can't initialize it yet because Ethernet init
    // blocks for too long.
}

void Bsp::initSysTick() noexcept {
    SysTick->LOAD = 168000U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void Bsp::initGpio() noexcept {
    RCC->AHB1ENR =
        RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOGEN;
    RCC->AHB1ENR;

    // USART1: PA9 and PB7, both use alternate function 7.
    GPIOA->AFR[1] = GPIO_AFRH_AFRH1_2 | GPIO_AFRH_AFRH1_1 | GPIO_AFRH_AFRH1_0;
    GPIOB->AFR[0] = GPIO_AFRL_AFRL7_2 | GPIO_AFRL_AFRL7_1 | GPIO_AFRL_AFRL7_0;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9) | GPIO_MODER_MODER9_1;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7) | GPIO_MODER_MODER7_1;

    // TODO: LED PWM: PB4, alternate function 2 (TIM1 channel 1).
    // GPIOB->AFR[0] |= GPIO_AFRL_AFRL4_1;
    // GPIOB->MODER |= GPIO_MODER_MODER4_0;

    // PC7: output to LED. TODO: remove.
    GPIOC->MODER = GPIO_MODER_MODER7_0;

    /* Ethernet pins (all use alternate function 11)
     * = high drive strength
     * PG2 = RMII_RXER
     * PG3 = EXT_RST
     * PG11 = ETH_TX_EN*
     * PG13 = ETH_TXD0*
     * PG14 = ETH_TXD1*
     * PC1 = ETH_MDC
     * PC4 = ETH_RXD0
     * PC5 = ETH_RXD1
     * PA1 = ETH_REF_CLK
     * PA2 = ETH_MDIO
     * PA7 = ETH_CRS_DV
     */
    GPIOA->MODER =
        (GPIOA->MODER & ~(GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER7_Msk)) |
        GPIO_MODER_MODER1_1 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER7_1;
    GPIOA->AFR[0] =
        (GPIOA->AFR[0] & ~(GPIO_AFRL_AFRL1_Msk | GPIO_AFRL_AFRL2_Msk | GPIO_AFRL_AFRL7_Msk)) |
        GPIO_AFRL_AFRL1_0 | GPIO_AFRL_AFRL1_1 | GPIO_AFRL_AFRL1_3 | GPIO_AFRL_AFRL2_0 |
        GPIO_AFRL_AFRL2_1 | GPIO_AFRL_AFRL2_3 | GPIO_AFRL_AFRL7_0 | GPIO_AFRL_AFRL7_1 |
        GPIO_AFRL_AFRL7_3;

    GPIOC->MODER =
        (GPIOC->MODER & ~(GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER4_Msk | GPIO_MODER_MODER5_Msk)) |
        GPIO_MODER_MODER1_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1;
    GPIOC->AFR[0] =
        (GPIOC->AFR[0] & ~(GPIO_AFRL_AFRL1_Msk | GPIO_AFRL_AFRL4_Msk | GPIO_AFRL_AFRL5_Msk)) |
        GPIO_AFRL_AFRL1_0 | GPIO_AFRL_AFRL1_1 | GPIO_AFRL_AFRL1_3 | GPIO_AFRL_AFRL4_0 |
        GPIO_AFRL_AFRL4_1 | GPIO_AFRL_AFRL4_3 | GPIO_AFRL_AFRL5_0 | GPIO_AFRL_AFRL5_1 |
        GPIO_AFRL_AFRL5_3;

    GPIOG->MODER = (GPIOG->MODER & ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER11_Msk |
                                     GPIO_MODER_MODER13_Msk | GPIO_MODER_MODER14_Msk)) |
                   GPIO_MODER_MODER2_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER13_1 |
                   GPIO_MODER_MODER14_1;
    GPIOG->AFR[0] = (GPIOG->AFR[0] & ~GPIO_AFRL_AFRL2_Msk) | GPIO_AFRL_AFRL2_0 | GPIO_AFRL_AFRL2_1 |
                    GPIO_AFRL_AFRL2_3;
    GPIOG->AFR[1] =
        (GPIOG->AFR[1] & ~(GPIO_AFRH_AFRH3_Msk | GPIO_AFRH_AFRH5_Msk | GPIO_AFRH_AFRH6_Msk)) |
        GPIO_AFRH_AFRH3_0 | GPIO_AFRH_AFRH3_1 | GPIO_AFRH_AFRH3_3 | GPIO_AFRH_AFRH5_0 |
        GPIO_AFRH_AFRH5_1 | GPIO_AFRH_AFRH5_3 | GPIO_AFRH_AFRH6_0 | GPIO_AFRH_AFRH6_1 |
        GPIO_AFRH_AFRH6_3;
    GPIOG->OSPEEDR =
        GPIO_OSPEEDR_OSPEEDR13_Msk | GPIO_OSPEEDR_OSPEEDR14_Msk | GPIO_OSPEEDR_OSPEEDR11_Msk;
}

void Bsp::initClocks() noexcept {
    // 168 MHz SYSCLK/AHB, 84 MHz APB2, and 42 MHz APB1 from the 16 MHz HSI.
    // Initialize the PLL with source=HSI, M=8, N=168, P=2, and Q=7.
    RCC->PLLCFGR = (7U << RCC_PLLCFGR_PLLQ_Pos) | (0U << RCC_PLLCFGR_PLLP_Pos) |
                   (168U << RCC_PLLCFGR_PLLN_Pos) | 8U;
    RCC->CR |= RCC_CR_PLLON;

    // Set voltage scale 2.
    RCC->APB1ENR = RCC_APB1ENR_PWREN;
    RCC->APB1ENR;
    PWR->CR1 = PWR_CR1_VOS_1;

    RCC->CFGR = RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4;
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;

    while ((PWR->CSR1 & PWR_CSR1_VOSRDY) == 0U) {
    }
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {
    }
    RCC->CFGR |= RCC_CFGR_SW_1;
}

// The address of this symbol is the base of the interrupt vector table.
extern "C" const uint32_t g_pfnVectors[];

void Bsp::initCpu() noexcept {
    SCB->VTOR = reinterpret_cast<uint32_t>(g_pfnVectors);
    SCB->SHCSR = SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

    // Enable the FPU with automatic CONTROL.FPCA management.
    SCB->CPACR = 0xFU << 20U;
    FPU->FPCCR = FPU_FPCCR_ASPEN_Msk;

    // TODO: enable MPU and ITCM.
    SCB->ICIALLU = 0U;
    __DSB();
    SCB->CCR = SCB_CCR_IC_Msk;
    __ISB();
}
