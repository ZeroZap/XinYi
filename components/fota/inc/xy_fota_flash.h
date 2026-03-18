/**
 * @file xy_fota_flash.h
 * @brief FOTA Flash Operations
 */

#ifndef XY_FOTA_FLASH_H
#define XY_FOTA_FLASH_H

#include <stdint.h>

typedef struct {
    int (*init)(void);
    int (*read)(uint32_t addr, uint8_t *buf, uint32_t len);
    int (*write)(uint32_t addr, const uint8_t *buf, uint32_t len);
    int (*erase)(uint32_t addr, uint32_t size);
} xy_fota_flash_ops_t;

#endif /* XY_FOTA_FLASH_H */
