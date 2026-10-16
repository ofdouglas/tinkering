#include <cstddef>
#include <cstdint>

#include "stm32f746xx.h"

#include "bare_metal/task.h"
#include "bsp.h"
#include "ethernet.h"
#include "hal/clock.h"
#include "logging/logging.h"
#include "uart.h"
#include "util/span.h"
#include "watchdog.h"

class LedBlinky : public bare_metal::PeriodicTask {
public:
  LedBlinky() : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)) {}

  void tick() noexcept override { GPIOC->ODR ^= GPIO_ODR_OD7_Msk; }
};

class UptimeLogger : public bare_metal::PeriodicTask {
public:
  UptimeLogger() : bare_metal::PeriodicTask(std::chrono::milliseconds(2000U)) {}

  void tick() noexcept override {
    LOG_INFO() << "Uptime ms: "
               << hal::SchedulerClock::now().time_since_epoch().count();
  }
};

template <size_t kNumPayloadBytes> struct EthernetPacket {
  uint8_t destination_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t source_mac[6];
  uint16_t ether_type;
  uint8_t payload[kNumPayloadBytes];
};

class EthernetTest : public bare_metal::PeriodicTask {
public:
  explicit EthernetTest(Ethernet &ethernet)
      : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)),
        ethernet_{ethernet} {}

  void tick() noexcept override {
    constexpr size_t kPayloadSize = 64U;

    EthernetPacket<kPayloadSize> packet{};
    packet.ether_type = kPayloadSize;
    for (size_t i = 0U; i < kPayloadSize; i++) {
      packet.payload[i] = static_cast<uint8_t>(i & 0xFFU);
    }

    const uint8_t *bytes = reinterpret_cast<uint8_t *>(&packet);
    ethernet_.send(util::Span(bytes, sizeof(packet)));

    tick_count_++;
    if (tick_count_ >= 10U) {
      tick_count_ = 0U;
      LOG_INFO() << "Eth TX frames: " << ETH->MMCTGFCR;
    }
  }

private:
  Ethernet &ethernet_;
  uint32_t tick_count_{};
};

class ResetCause {
public:
  ResetCause() noexcept {
    value_ = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;
  }

  void log() noexcept {
    LOG_INFO() << "Reset Cause:" << ((value_ & RCC_CSR_LPWRRSTF) ? " LPWR" : "")
               << ((value_ & RCC_CSR_WWDGRSTF) ? " WWDG" : "")
               << ((value_ & RCC_CSR_IWDGRSTF) ? " IWDG" : "")
               << ((value_ & RCC_CSR_SFTRSTF) ? " SFT" : "")
               << ((value_ & RCC_CSR_PORRSTF) ? " POR" : "")
               << ((value_ & RCC_CSR_PINRSTF) ? " PIN" : "")
               << ((value_ & RCC_CSR_BORRSTF) ? " BOR" : "");
  }

private:
  uint32_t value_;
};

int main() {
  Bsp bsp{};

  // TODO: make these BSP modules
  ResetCause reset_cause{};
  [[maybe_unused]] Watchdog watchdog{};
  Uart uart{};
  static Ethernet ethernet{};

  // Tasks
  LedBlinky led_blinky{};
  EthernetTest eth_task{ethernet};
  UptimeLogger uptime_logger{};

  // watchdog.init();
  bsp.init();
  uart.init();

  // watchdog.kick();

  logging::setLogSink(&uart);
  reset_cause.log();
  __enable_irq();

  ethernet.init();
  // TODO: add PWM initialization to Bsp.

  led_blinky.start();
  uptime_logger.start();
  eth_task.start();

  for (;;) {
    // watchdog.kick();
    led_blinky.poll();
    uptime_logger.poll();
    eth_task.poll();
  }

  while (true) {
  }
}
