#ifndef XY_W25Q128_H
#define XY_W25Q128_H

#include "xy_device.h"
#include "xy_hal_qspi.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XY_W25Q128_OK = 0,
    XY_W25Q128_ERROR = -1,
    XY_W25Q128_INVALID_PARAM = -2,
    XY_W25Q128_NOT_FOUND = -3,
    XY_W25Q128_TIMEOUT = -4,
} xy_w25q128_status_t;

typedef struct {
    xy_device_t device;
    void *qspi;
    uint32_t jedec_id;
    uint32_t capacity;
    uint8_t initialized;
} xy_w25q128_t;

xy_w25q128_status_t xy_w25q128_init(xy_w25q128_t *flash, void *qspi, const char *name);
xy_w25q128_status_t xy_w25q128_read(xy_w25q128_t *flash, uint32_t address, uint8_t *data,
                                    size_t length);
xy_w25q128_status_t xy_w25q128_quad_read(xy_w25q128_t *flash, uint32_t address, uint8_t *data,
                                         size_t length, uint8_t dummy_cycles);
xy_w25q128_status_t xy_w25q128_sector_erase(xy_w25q128_t *flash, uint32_t address,
                                            uint32_t timeout_ms);
xy_w25q128_status_t xy_w25q128_page_program(xy_w25q128_t *flash, uint32_t address,
                                            const uint8_t *data, size_t length,
                                            uint32_t timeout_ms);
xy_w25q128_status_t xy_w25q128_quad_page_program(xy_w25q128_t *flash, uint32_t address,
                                                 const uint8_t *data, size_t length,
                                                 uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
#endif
