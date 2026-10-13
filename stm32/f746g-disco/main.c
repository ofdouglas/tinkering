#include <stdint.h>
#include <stddef.h>

#include "stm32f746xx.h"
#include "core_cm7.h"

volatile uint32_t uptime_ms;

void SysTick_Handler(void) {
  if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U) {
    uptime_ms++;
  }
}

void delay_for(uint32_t milliseconds) {
  const uint32_t end = uptime_ms + milliseconds;
  while (uptime_ms < end) {} // wait
}

void uart_putchar(char c) {
  while ((USART1->ISR & USART_ISR_TXE) == 0U) {} // wait
  USART1->TDR = c;
}

void uart_puts(const char* str) {
  while (*str != '\0') {
    uart_putchar(*str++);
  }
}

void bsp_init_systick(void) {
  SysTick->VAL = 0;
  SysTick->LOAD = 168000;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void bsp_init_gpio(void) {
  RCC->AHB1ENR = RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOAEN;
  RCC->AHB1ENR;

  // USART1: PA9 and PB7, both use alternate function 7
  GPIOA->AFR[1] = GPIO_AFRH_AFRH1_2 | GPIO_AFRH_AFRH1_1 | GPIO_AFRH_AFRH1_0;
  GPIOB->AFR[0] = GPIO_AFRL_AFRL7_2 | GPIO_AFRL_AFRL7_1 | GPIO_AFRL_AFRL7_0;
  GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9) | GPIO_MODER_MODER9_1;
  GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7) | GPIO_MODER_MODER7_1;

  // PC7 : output to LED
  GPIOC->MODER = GPIO_MODER_MODER7_0;
}

void bsp_init_uart(void) {
  RCC->APB2ENR = RCC_APB2ENR_USART1EN;
  RCC->APB2ENR;

  USART1->BRR = 4375; // 19.2 kbps from 84 MHz APB2 clock
  USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

// 168 MHz Sysclk / AHB, 84 MHz APB2, 42 MHz APB1, from HSI 16 MHz
void bsp_init_clocks(void) {
  // Initialize the PLL. Src=HSI-16, M=8, N=168, P=2, Q=7.
  // Use PLLP[1:0] == 0 to select PLL_P = 2
  RCC->PLLCFGR = (7U << RCC_PLLCFGR_PLLQ_Pos) | (0U << RCC_PLLCFGR_PLLP_Pos) | (168U << RCC_PLLCFGR_PLLN_Pos) | 8U;
  RCC->CR |= RCC_CR_PLLON;

  // Set voltage scale 2
  RCC->APB1ENR = RCC_APB1ENR_PWREN;
  RCC->APB1ENR;
  PWR->CR1 = PWR_CR1_VOS_1;

  // Set peripheral bus dividers
  RCC->CFGR = RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4; // AHB not divided

  // Configure flash wait states
  FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;

  // Switch Sysclk to PLL when ready
  while ((PWR->CSR1 & PWR_CSR1_VOSRDY) == 0U) {} // wait
  while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {} // wait
  RCC->CFGR |= RCC_CFGR_SW_1; // Sysclk = PLL
}

// The address of this symbol is the base of the interrupt vector table.
extern const uint32_t g_pfnVectors[];

// TODO: enable MPU and ITCM
void bsp_init_cpu(void) {
  // Relocate the vector table and enable fault handlers
  SCB->VTOR = (uint32_t)g_pfnVectors;
  SCB->SHCSR = SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

  // Enable FPU (cp10, cp11), with automatic CONTROL.FPCA management
  SCB->CPACR = 0xFU << 20U;
  FPU->FPCCR = FPU_FPCCR_ASPEN_Msk;

  // Enable I-cache
  SCB->ICIALLU = 0U;
  __DSB();
  SCB->CCR = SCB_CCR_IC_Msk;
  __ISB();
}

int main(void) {
  bsp_init_cpu();
  bsp_init_clocks();
  bsp_init_systick();
  bsp_init_gpio();
  bsp_init_uart();

  GPIOC->ODR = GPIO_ODR_OD7_Msk;

  uint32_t counter = 0;

  for (;;) {
    delay_for(500U);
    GPIOC->ODR ^= GPIO_ODR_OD7_Msk;

    if (counter >= 5U) {
      counter = 0U;
      uart_puts("Hello, world!\n");
    } else {
      counter++;
    }
  }

  while (1) {}
}
