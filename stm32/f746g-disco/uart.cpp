#include "uart.h"

#include "osal/critical_section.h"

namespace {
Uart *uart_instance_{nullptr};
}

bool Uart::startDma() noexcept {
    if (dma_current_size_.load() != 0U) {
        return true;  // DMA already active
    }

    util::Span<const uint8_t> payload_view{uart_tx_buffer_.readBulk()};
    if (payload_view.size() == 0U) {
        return false;  // No data to send
    }

    dma_current_size_.store(payload_view.size());
    DMA2_Stream7->M0AR = reinterpret_cast<uint32_t>(payload_view.data());
    DMA2_Stream7->NDTR = payload_view.size();
    DMA2_Stream7->CR |= DMA_SxCR_EN;
    return true;
}

void Uart::stopDma() noexcept {
    DMA2->HIFCR |= DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7;

    uart_tx_buffer_.discardBulk(dma_current_size_.load());
    dma_current_size_.store(0U);
}

extern "C" {
void DMA2_Stream7_IRQHandler() {
    if (uart_instance_ == nullptr) {
        // TODO: log error
        return;
    }
    uart_instance_->handleInterrupt();
}
}

void Uart::handleInterrupt() noexcept {
    const uint32_t hisr{DMA2->HISR};

    stopDma();
    startDma();

    if ((hisr & (DMA_HISR_TEIF7 | DMA_HISR_DMEIF7)) != 0) {
        dma_error_flags_.store(hisr);
    }
}

size_t Uart::read(util::Span<uint8_t> data) noexcept {
    (void)data;
    return 0U;
}

size_t Uart::write(util::Span<const uint8_t> data) noexcept {
    osal::RaiiCriticalSection cs{};
    if (!uart_tx_buffer_.enqueue(data)) {
        return 0U;
    }
    (void)startDma();
    return data.size();
}

// USART1 DMA channels:
// DMA2 Stream 5 Channel 4: RX
// DMA2 Stream 7 Channel 4: TX
void Uart::init() noexcept {
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB2ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    RCC->AHB1ENR;

    // Memory -> USART TDR, direct mode, single bytes
    DMA2_Stream7->PAR = USART1_BASE + offsetof(USART_TypeDef, TDR);
    DMA2_Stream7->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_MINC | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE |
                       DMA_SxCR_TEIE | DMA_SxCR_DMEIE;

    NVIC_SetPriority(DMA2_Stream7_IRQn, 7U);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    USART1->BRR = 84;  // 1 Mbps from 84 MHz APB2 clock
    USART1->CR3 |= USART_CR3_DMAT;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    uart_instance_ = this;
}
