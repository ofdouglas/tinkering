#pragma once
/*
 * @file spi.h
 * @brief Blocking driver for the system SPI controller.
 */

#include <stddef.h>
#include <stdint.h>

void spi_init(void);

/**
 * @brief Exchange one byte while leaving chip select asserted.
 *
 * Call spi_end_transaction() after the final byte in the transaction.
 */
uint8_t spi_transfer_byte_blocking(uint8_t data);

void spi_end_transaction(void);

/**
 * @brief Exchange a byte buffer as one chip-select transaction.
 *
 * @param[in] tx_data Bytes to transmit. Must be non-null when length is nonzero.
 * @param[out] rx_data Received bytes, or null to discard received data.
 * @param[in] length Number of bytes to exchange.
 */
void spi_transfer_blocking(const uint8_t* tx_data, uint8_t* rx_data, size_t length);
