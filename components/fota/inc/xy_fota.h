/**
 * @file xy_fota.h
 * @brief Firmware Over-The-Air Update
 */

#ifndef XY_FOTA_H
#define XY_FOTA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t version;
    uint32_t size;
    uint8_t checksum[32];
} xy_fota_header_t;

int xy_fota_init(void);
int xy_fota_start(const uint8_t *firmware, uint32_t size);
int xy_fota_verify(void);
int xy_fota_apply(void);

#endif /* XY_FOTA_H */
