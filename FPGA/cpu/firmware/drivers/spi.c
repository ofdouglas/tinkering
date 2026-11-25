#include "drivers/spi.h"
#include "linker/mem_map.h"

#include <stddef.h>
#include <stdint.h>

#define SPI_CONTROL_REG ((volatile uint32_t*)(SPI_BASE + 0))
#define SPI_STATUS_REG ((volatile uint32_t*)(SPI_BASE + 8))
#define SPI_RX_DATA_REG ((volatile uint32_t*)(SPI_BASE + 12))
#define SPI_TX_DATA_REG ((volatile uint8_t*)(SPI_BASE + 16))

#define SPI_CONTROL_ENABLE (1U << 0)

#define SPI_STATUS_TX_READY (1U << 0)
#define SPI_STATUS_RX_VALID (1U << 1)
#define SPI_STATUS_CS_ACTIVE (1U << 2)

void spi_init(void) {
    *SPI_CONTROL_REG = SPI_CONTROL_ENABLE;
    *SPI_STATUS_REG = SPI_STATUS_CS_ACTIVE;
}

uint8_t spi_transfer_byte_blocking(uint8_t data) {
    while (!(*SPI_STATUS_REG & SPI_STATUS_TX_READY)) {
        ;
    }
    *SPI_TX_DATA_REG = data;

    while (!(*SPI_STATUS_REG & SPI_STATUS_RX_VALID)) {
        ;
    }
    return (uint8_t)*SPI_RX_DATA_REG;
}

void spi_end_transaction(void) {
    *SPI_STATUS_REG = SPI_STATUS_CS_ACTIVE;
}

void spi_transfer_blocking(const uint8_t* tx_data, uint8_t* rx_data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        const uint8_t received = spi_transfer_byte_blocking(tx_data[i]);
        if (rx_data != NULL) {
            rx_data[i] = received;
        }
    }

    if (length != 0U) {
        spi_end_transaction();
    }
}
