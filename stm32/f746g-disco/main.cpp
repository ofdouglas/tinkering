#include <cstddef>
#include <cstdint>
#include <exception>

#include "stm32f746xx.h"

#include "bare_metal/task.h"
#include "bsp.h"
#include "ethernet.h"
#include "hal/reset_manager_interface.h"
#include "logging/logging.h"

class LedBlinky : public bare_metal::PeriodicTask {
public:
    LedBlinky() : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)) {}

    void tick() noexcept override {
        GPIOC->ODR ^= GPIO_ODR_OD7_Msk;
    }
};

class UptimeLogger : public bare_metal::PeriodicTask {
public:
    UptimeLogger() : bare_metal::PeriodicTask(std::chrono::milliseconds(2000U)) {}

    void tick() noexcept override {
        LOG_INFO() << "Uptime: " << hal::SchedulerClock::now();
    }
};

template <size_t kNumPayloadBytes>
struct EthernetPacket {
    uint8_t destination_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t source_mac[6];
    uint16_t ether_type;
    uint8_t payload[kNumPayloadBytes];
};

class EthernetTest : public bare_metal::PeriodicTask {
public:
    explicit EthernetTest(Ethernet &ethernet)
        : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)), ethernet_{ethernet} {}

    void tick() noexcept override {
        constexpr size_t kPayloadSize = 64U;

        if (ethernet_.getState() != Ethernet::State::kLinkUp) {
            return;
        }

        auto packet_handle = ethernet_.allocatePacketBuffer();
        if (packet_handle == nullptr) {
            LOG_ERROR() << "Ethernet TX buffer allocation failed";
            return;
        }

        packet_handle->length = kPayloadSize;
        for (size_t i = 0U; i < kPayloadSize; i++) {
            packet_handle->bytes[i] = static_cast<uint8_t>(i & 0xFFU);
        }

        ethernet_.send(std::move(packet_handle));

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

class EthernetLinkManager : public bare_metal::PeriodicTask {
public:
    explicit EthernetLinkManager(Ethernet &ethernet)
        : bare_metal::PeriodicTask(std::chrono::milliseconds(50U)), ethernet_{ethernet} {}

    void tick() noexcept override {
        const Ethernet::State new_state{ethernet_.runLinkManagement()};

        if ((new_state == Ethernet::State::kError) && (new_state != prev_state_)) {
            LOG_ERROR() << "Ethernet entered error state";
        }
        prev_state_ = new_state;
    }

private:
    Ethernet &ethernet_;
    Ethernet::State prev_state_{};
};

// TODO: add a terminate reason enum.
// TODO: make this a common library? needs platform-specific IRQ disable.
[[noreturn]] void mcuTerminate() noexcept {
    __disable_irq();

    // TODO: record crash dump.

    // Immediate reset, because the watchdog might not be enabled.
    NVIC_SystemReset();
    __builtin_unreachable();
}

int main() {
    static Bsp bsp{};
    static Ethernet ethernet{};

    std::set_terminate(mcuTerminate);

    // Tasks. TODO: organize into an application class, store in .bss section.
    LedBlinky led_blinky{};
    EthernetTest eth_task{ethernet};
    EthernetLinkManager link_manager{ethernet};
    UptimeLogger uptime_logger{};

    bsp.init();
    ethernet.init();

    logging::setLogSink(bsp.getUart());
    bsp.getResetManager().logResetCause();
    __enable_irq();

    led_blinky.start();
    uptime_logger.start();
    eth_task.start();
    link_manager.start();

    for (;;) {
        led_blinky.poll();
        uptime_logger.poll();
        eth_task.poll();
        link_manager.poll();
    }

    std::terminate();
    __builtin_unreachable();
}
