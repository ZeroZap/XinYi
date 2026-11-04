/**
 * @file xy_spi.h
 * @brief SPI Interface Placeholder
 */

#ifndef XY_SPI_H
#define XY_SPI_H

#include <stdint.h>

typedef void xy_spi_t;

int xy_spi_transfer(xy_spi_t *spi, uint8_t cs, const uint8_t *tx, uint16_t tx_len,
                    uint8_t *rx, uint16_t rx_len, uint32_t timeout_ms);

#endif /* XY_SPI_H */
