/**
  ******************************************************************************
  * @file    main.cpp
  * @brief   Bootloader main program
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"

#include "bootloader/image_header.h"
#include "bootloader/mcu/bootloader.h"
#include "bootloader/transport/hdlc_transport.h"

#include "hal/clock.h"
#include "hal/delay.h"
#include "logging/logging.h"
#include "data_structures/ring_buffer.h"
#include "hdlc/hdlc.h"
#include "hdlc/protocol.h"

#include "stm32f7xx_it.h"
#include "stm32f746xx.h"

///////////////////////////////////////////////////////////////////////////////
// Transport layer
///////////////////////////////////////////////////////////////////////////////

constexpr size_t kUartRxRingCapacity{256U};

uint8_t uart_rx_char{};
uint32_t uart_isr_count{};
uint8_t num_enqueue_errors{};
RingBuffer<uint8_t, kUartRxRingCapacity> ring_buffer{};

extern "C" {
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* uart) {
    uart_isr_count++;

    num_enqueue_errors += ring_buffer.enqueue(uart_rx_char) ? 0U : 1U;

    __HAL_UART_ENABLE_IT(uart, UART_IT_RXNE);
    HAL_UART_Receive_IT(uart, &uart_rx_char, 1U);
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&bsp.huart1);
}
} // extern "C"


class UartHdlcStream : public Stream::StreamInterface {
public:
    explicit UartHdlcStream(UART_HandleTypeDef& uart, RingBuffer<uint8_t, kUartRxRingCapacity>& ring_buffer) noexcept
        : uart_(uart), ring_buffer_(ring_buffer) {}

    size_t read(util::Span<uint8_t> data) noexcept override {
        return ring_buffer_.dequeue(data);
    }
    
    size_t write(util::Span<const uint8_t> data) noexcept override {
        return HAL_UART_Transmit(&uart_, const_cast<uint8_t*>(data.data()), data.size(), 100U) == HAL_OK;
    }

private:
    UART_HandleTypeDef& uart_;
    RingBuffer<uint8_t, kUartRxRingCapacity>& ring_buffer_;
};

UartHdlcStream uart_hdlc_stream{bsp.huart1, ring_buffer};
bootloader::HdlcTransport<> hdlc_transport{uart_hdlc_stream};
bootloader::Bootloader bootloader_app{bsp.memory_regions, hdlc_transport, bsp.system_reset};

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


    bootloader_app.initialize();
    LOG_INFO() << "Bootloader started.";

    if (bootloader_app.validateApplication()) {
        LOG_INFO() << "Bootable application found";
    } else {
        LOG_INFO() << "No bootable application found";
    }

    while (true) {
        // tick() will eventually load the app if it is valid
        if (bootloader_app.tick() == bootloader::Bootloader::State::kFault) {
            LOG_FATAL() << "Bootloader fault";
            return 2;
        }
    }

    while (true) {} // Shouldn't get here
    return 3;
}
