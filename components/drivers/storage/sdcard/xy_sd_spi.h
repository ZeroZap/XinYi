#ifndef XY_SD_SPI_H
#define XY_SD_SPI_H

#include "xy_hal_error.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_SD_SPI_BLOCK_SIZE 512U

typedef xy_hal_error_t (*xy_sd_spi_transfer_fn)(void *spi, const uint8_t *tx, uint8_t *rx,
                                                size_t length, uint32_t timeout_ms);
typedef xy_hal_error_t (*xy_sd_spi_cs_fn)(void *arg, uint8_t level);
typedef void (*xy_sd_spi_delay_fn)(uint32_t ms);

typedef enum {
    XY_SD_SPI_CARD_UNKNOWN = 0,
    XY_SD_SPI_CARD_SDSC,
    XY_SD_SPI_CARD_SDHC,
} xy_sd_spi_card_type_t;

typedef struct {
    void *spi;
    void *cs_arg;
    xy_sd_spi_transfer_fn transfer;
    xy_sd_spi_cs_fn set_cs;
    xy_sd_spi_delay_fn delay_ms;
    uint32_t timeout_ms;
} xy_sd_spi_config_t;

typedef struct {
    xy_sd_spi_config_t config;
    xy_sd_spi_card_type_t type;
    uint64_t capacity_bytes;
    uint32_t block_count;
    uint8_t initialized;
} xy_sd_spi_t;

xy_hal_error_t xy_sd_spi_init(xy_sd_spi_t *card, const xy_sd_spi_config_t *config);
xy_hal_error_t xy_sd_spi_read_block(xy_sd_spi_t *card, uint32_t block, uint8_t *data);
xy_hal_error_t xy_sd_spi_write_block(xy_sd_spi_t *card, uint32_t block, const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
