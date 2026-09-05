/**
 * @file xy_hal_qspi.h
 * @brief Quad-SPI hardware abstraction
 */
#ifndef XY_HAL_QSPI_H
#define XY_HAL_QSPI_H

#include "xy_hal_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XY_HAL_QSPI_LINES_NONE = 0,
    XY_HAL_QSPI_LINES_1 = 1,
    XY_HAL_QSPI_LINES_2 = 2,
    XY_HAL_QSPI_LINES_4 = 4,
} xy_hal_qspi_lines_t;

typedef struct {
    uint32_t clock_prescaler;
    uint32_t fifo_threshold;
    uint32_t flash_size_bits;
    uint32_t chip_select_high_cycles;
    uint8_t sample_shift_half_cycle;
    uint8_t clock_mode;
} xy_hal_qspi_config_t;

typedef struct {
    uint8_t instruction;
    uint8_t has_address;
    uint32_t address;
    uint8_t address_bits;
    xy_hal_qspi_lines_t instruction_lines;
    xy_hal_qspi_lines_t address_lines;
    xy_hal_qspi_lines_t data_lines;
    uint8_t dummy_cycles;
    size_t data_length;
    bool write;
} xy_hal_qspi_command_t;

xy_hal_error_t xy_hal_qspi_init(void *qspi, const xy_hal_qspi_config_t *config);
xy_hal_error_t xy_hal_qspi_deinit(void *qspi);
xy_hal_error_t xy_hal_qspi_command(void *qspi, const xy_hal_qspi_command_t *command,
                                   uint8_t *data, uint32_t timeout);
uint32_t xy_hal_qspi_tick_ms(void);

#ifdef __cplusplus
}
#endif
#endif
