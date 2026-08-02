/**
  ******************************************************************************
  * @file    main.cpp
  * @brief   Bootloader main program
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"

#include "bare_metal/task.h"
#include "bare_metal/scheduler.h"
#include "bootloader/image_header.h"
#include "hal/clock.h"
#include "hal/delay.h"
#include "logging/log.hpp"
#include "data_structures/ring_buffer.h"
#include "hdlc/hdlc.h"

#include "stm32f7xx_it.h"
#include "stm32f746xx.h"

///////////////////////////////////////////////////////////////////////////////
// Task implementations
///////////////////////////////////////////////////////////////////////////////

volatile size_t uart_isr_count{};
volatile size_t uart_task_count{};


class LogTimeTask : public bare_metal::PeriodicTask {
public:
    LogTimeTask() : bare_metal::PeriodicTask(std::chrono::milliseconds(2000U)) {}

    void tick() noexcept override {
        const auto now = hal::SchedulerClock::now();
        LOG_INFO() << "LogTimeTask tick. Scheduler clock: " << now.time_since_epoch().count();
        LOG_INFO() << "UART ISR / Task count: " << uart_isr_count << " / " << uart_task_count;
    }
};

uint8_t uart_rx_char{};
RingBuffer<uint8_t, 64U> uart_rx_buffer{};

extern "C" {
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* uart) {
    uart_isr_count++;

    volatile char data = uart_rx_char;
    uart_rx_buffer.enqueue(data);

    __HAL_UART_ENABLE_IT(uart, UART_IT_RXNE);
    HAL_UART_Receive_IT(uart, &uart_rx_char, 1U);
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&bsp.huart1);
}
} // extern "C"

class UartTask :  public bare_metal::PeriodicTask {
public:
    UartTask() : bare_metal::PeriodicTask(std::chrono::milliseconds(100U)) {}

    void tick() noexcept override {
        uint8_t next{};
        while (index_ < (buffer_.size() + 1)) {
            if (!uart_rx_buffer.dequeue(next)) {
                break;
            }

            uart_task_count++;
            buffer_[index_++] = static_cast<char>(next);
            
            if (next == '\n') {
                buffer_[index_] = '\0';
                LOG_INFO() << reinterpret_cast<char*>(buffer_.data());
                index_ = 0;
            }
        }
    }

private:
    std::array<char, 64U> buffer_{};
    size_t index_{};
};


void runTasks() noexcept {
    LogTimeTask log_time_task{};
    UartTask uart_task{};
    bare_metal::Scheduler<2U> scheduler{};

    scheduler.addTask(&log_time_task);
    scheduler.addTask(&uart_task);
    scheduler.start();
    scheduler.run();    // Does not return
}

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////
int main(void) {
    if (!bsp.earlyInit()) {
        LOG_FATAL() << "BSP initialization failed";
        return 1;
    }
    
    // testFlash();

    HAL_UART_Receive_IT(&bsp.huart1, &uart_rx_char, 1U);
    LOG_INFO() << "Bootloader started.";
    runTasks();

    while (true) {} // Shouldn't get here
    return 2;
}
