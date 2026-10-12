#include "stm32f746xx.h"
#include "stm32f7xx.h"
#include "core_cm7.h"


// 168 MHz Sysclk / AHB, 84 MHz APB2, 42 MHz APB1, from HSI 16 MHz
void bsp_init_clocks(void) {
  FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;

  PWR->CR1 = PWR_CR1_VOS_1;
  // while ((PWR->CSR1 & PWR_CSR1_VOSRDY) == 0U) {} // wait

  RCC->CFGR = RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4; // AHB not divided

  // PLL Src=HSI-16, N=168, M=8, P=2, Q=7. Use PLLP[1:0] == 0 to select PLL_P = 2
  RCC->PLLCFGR = (7U << RCC_PLLCFGR_PLLQ_Pos) | (0U << RCC_PLLCFGR_PLLP_Pos) | (168U << RCC_PLLCFGR_PLLN_Pos) | 8U;
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {} // wait

  RCC->CFGR |= RCC_CFGR_SW_1; // Sysclk = PLL
}

/* 
   LED = D0 = PC7 
   VCP_RX = PB7
   VCP_TX = PA9
*/
void bsp_init_gpio(void) {
  RCC->AHB1ENR = RCC_AHB1ENR_GPIOCEN_Msk | RCC_AHB1ENR_GPIOBEN_Msk | RCC_AHB1ENR_GPIOAEN_Msk;

  GPIOC->MODER; // Dummy read to delay the GPIO access so it has time to power up
  GPIOC->MODER = GPIO_MODER_MODER7_0; // PC7 output to LED
  // TODO init UART GPIOs
}

int main(void) {
  bsp_init_clocks();
  bsp_init_gpio();
  // init UART

  GPIOC->ODR = GPIO_ODR_OD7_Msk;

  SysTick->LOAD = 0xFFFFFFUL;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

  for (;;) {
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {} // wait;
    GPIOC->ODR ^= GPIO_ODR_OD7_Msk;
  }
}
