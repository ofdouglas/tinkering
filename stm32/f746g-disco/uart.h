#pragma once

#include <atomic>
#include <cstdint>

#include "data_structures/ring_buffer.h"
#include "logging/logging.h"
#include "stm32f746xx.h"
#include "util/span.h"

class Uart : public logging::LogSink {
 public:
  Uart() noexcept = default;
  ~Uart() noexcept override = default;

  Uart(const Uart &) = delete;
  Uart &operator=(const Uart &) = delete;
  Uart(Uart &&) noexcept = delete;
  Uart &operator=(Uart &&) noexcept = delete;

  void init() noexcept;
  bool write(util::Span<const uint8_t> message) noexcept override;
  void handleInterrupt() noexcept;

 private:
  bool startDma() noexcept;
  void stopDma() noexcept;

  RingBuffer<uint8_t, 512U> uart_tx_buffer_{};
  std::atomic<uint32_t> dma_current_size_{};
  std::atomic<uint32_t> dma_error_flags_{};
};