#include <algorithm>
#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>

#include "stm32f746xx.h"
#include "core_cm7.h"

#include "bare_metal/task.h"
#include "data_structures/ring_buffer.h"
#include "logging/logging.h"
#include "hal/clock.h"
#include "hal/delay.h"
#include "util/span.h"

std::atomic<uint32_t> sys_tick_count{};

extern "C" void SysTick_Handler() {
  sys_tick_count++;
}

hal::SchedulerClock::TimePoint hal::SchedulerClock::now() noexcept {
  return hal::SchedulerClock::TimePoint(std::chrono::milliseconds(sys_tick_count));
}

void bsp_init_systick() {
  SysTick->LOAD = 168000;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void bsp_init_watchdog() {
  IWDG->KR = 0x5555U;
  IWDG->KR = 0xCCCCU;
}

void bsp_watchdog_kick() {
  IWDG->KR = 0xAAAA;
}

void bsp_init_gpio() {
  RCC->AHB1ENR = RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOAEN;
  RCC->AHB1ENR;

  // USART1: PA9 and PB7, both use alt. function 7
  GPIOA->AFR[1] = GPIO_AFRH_AFRH1_2 | GPIO_AFRH_AFRH1_1 | GPIO_AFRH_AFRH1_0;
  GPIOB->AFR[0] = GPIO_AFRL_AFRL7_2 | GPIO_AFRL_AFRL7_1 | GPIO_AFRL_AFRL7_0;
  GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9) | GPIO_MODER_MODER9_1;
  GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7) | GPIO_MODER_MODER7_1;

  // TODO: LED PWM: PB4, alt. function 2 (TIM1 ch.1)
  // GPIOB->AFR[0] |= GPIO_AFRL_AFRL4_1;
  // GPIOB->MODER |= GPIO_MODER_MODER4_0;

  // PC7 : output to LED (TODO: remove)
  GPIOC->MODER = GPIO_MODER_MODER7_0;
}

// USART1 DMA channels:
// DMA2 Stream 5 Channel 4: RX
// DMA2 Stream 7 Channel 4: TX
void bsp_init_uart() {
  RCC->APB2ENR = RCC_APB2ENR_USART1EN;
  RCC->APB2ENR;

  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
  RCC->AHB1ENR;

  // Memory -> USART TDR, direct mode, single bytes
  DMA2_Stream7->PAR = USART1_BASE + offsetof(USART_TypeDef, TDR);
  DMA2_Stream7->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_MINC | DMA_SxCR_DIR_0
                          | DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE;

  NVIC_SetPriority(DMA2_Stream7_IRQn, 7U);
  NVIC_EnableIRQ(DMA2_Stream7_IRQn);

  USART1->BRR = 4375; // 19.2 kbps from 84 MHz APB2 clock
  USART1->CR3 |= USART_CR3_DMAT;
  USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

RingBuffer<uint8_t, 128> uart_tx_buffer{};
std::atomic<uint32_t> dma_current_size{};
std::atomic<uint32_t> dma_error_flags{};

bool bsp_uart_start_dma() {
  if (dma_current_size.load() != 0U) {
    return true; // DMA already active
  }

  util::Span<const uint8_t> payload_view{uart_tx_buffer.readBulk()};
  if (payload_view.size() == 0U) {
    return false; // No data to send
  }

  dma_current_size.store(payload_view.size());
  DMA2_Stream7->M0AR = reinterpret_cast<uint32_t>(payload_view.data());
  DMA2_Stream7->NDTR = payload_view.size();
  DMA2_Stream7->CR |= DMA_SxCR_EN;
  return true;
}

void bsp_uart_stop_dma() {
  DMA2->HIFCR |= DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7;

  uart_tx_buffer.discardBulk(dma_current_size.load());
  dma_current_size.store(0U);
}

extern "C" {
void DMA2_Stream7_IRQHandler() {
  const uint32_t hisr{DMA2->HISR};

  bsp_uart_stop_dma();
  bsp_uart_start_dma();

  if ((hisr & (DMA_HISR_TEIF7 | DMA_HISR_DMEIF7)) != 0) {
    dma_error_flags.store(hisr);
  }
}
}

void bsp_uart_send(util::Span<const uint8_t> payload) {
  uart_tx_buffer.enqueue(payload);

  // TODO: proper critical section
  __disable_irq();
  bsp_uart_start_dma();
  __enable_irq();
}

// 168 MHz Sysclk / AHB, 84 MHz APB2, 42 MHz APB1, from HSI 16 MHz
void bsp_init_clocks() {
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
extern "C" const uint32_t g_pfnVectors[];

// TODO: enable MPU and ITCM
void bsp_init_cpu() {
  // Relocate the vector table and enable fault handlers
  SCB->VTOR = reinterpret_cast<uint32_t>(g_pfnVectors);
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

/**
 * @brief Logging sink that writes messages synchronously to USART1.
 */
class UartLogSink final : public logging::LogSink {
public:
  bool write(util::Span<const uint8_t> message) override {
    bsp_uart_send(message);
    return true;
  }
};

class LedBlinky : public bare_metal::PeriodicTask {
public:
  LedBlinky() : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)) {}

  void tick() noexcept override  {
    GPIOC->ODR ^= GPIO_ODR_OD7_Msk;
  }
};

class UptimeLogger : public bare_metal::PeriodicTask {
  public:
  UptimeLogger() : bare_metal::PeriodicTask(std::chrono::milliseconds(2000U)) {}

    void tick() noexcept override {
      LOG_INFO() << "Uptime ms: " << hal::SchedulerClock::now().time_since_epoch().count();
    }
  };


class ResetCause {
public:
  ResetCause() noexcept {
    value_ = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;
  }

  void log() noexcept {
    LOG_INFO() << "Reset Cause:" << ((value_ & RCC_CSR_LPWRRSTF) ? " LPWR" : "")
               << ((value_ & RCC_CSR_WWDGRSTF) ? " WWDG" : "") << ((value_ & RCC_CSR_IWDGRSTF) ? " IWDG" : "")
               << ((value_ & RCC_CSR_SFTRSTF) ? " SFT" : "") << ((value_ & RCC_CSR_PORRSTF) ? " POR" : "")
               << ((value_ & RCC_CSR_PINRSTF) ? " PIN" : "") << ((value_ & RCC_CSR_BORRSTF) ? " BOR" : "");
  }

private:
  uint32_t value_;
};

int main() {
  ResetCause reset_cause{};
  bsp_init_watchdog();
  bsp_init_cpu();
  bsp_init_clocks();
  bsp_init_systick();
  bsp_init_gpio();
  bsp_init_uart();
  // bsp_init_pwm();

  bsp_watchdog_kick();
  __enable_irq();

  UartLogSink uart_log_sink{};
  LedBlinky led_blinky{};
  UptimeLogger uptime_logger{};

  logging::setLogSink(&uart_log_sink);
  led_blinky.start();
  uptime_logger.start();
  reset_cause.log();

  for (;;) {
    bsp_watchdog_kick();
    led_blinky.poll();
    uptime_logger.poll();
  }

  while (true) {}
}
