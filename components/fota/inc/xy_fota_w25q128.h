#ifndef XY_FOTA_W25Q128_H
#define XY_FOTA_W25Q128_H

#include "xy_fota.h"
#include "xy_w25q128.h"

#ifdef __cplusplus
extern "C" {
#endif

int xy_fota_w25q128_bind(xy_w25q128_t *flash, uint32_t base, uint32_t size,
                          uint32_t timeout_ms);
const xy_fota_flash_ops_t *xy_fota_w25q128_ops(void);

#ifdef __cplusplus
}
#endif
#endif
