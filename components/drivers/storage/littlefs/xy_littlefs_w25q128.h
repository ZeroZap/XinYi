#ifndef XY_LITTLEFS_W25Q128_H
#define XY_LITTLEFS_W25Q128_H

#include "lfs.h"
#include "xy_w25q128.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_LITTLEFS_W25Q128_DEFAULT_BASE 0U
#define XY_LITTLEFS_W25Q128_DEFAULT_SIZE 0x00F00000U

typedef struct {
    lfs_t fs;
    struct lfs_config config;
    xy_w25q128_t* flash;
    uint8_t mounted;
    uint32_t base;
    uint32_t size;
} xy_littlefs_w25q128_t;

/** Mount the default filesystem partition below the reserved FOTA region. */
int xy_littlefs_w25q128_mount(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash);
/** Format and mount the default filesystem partition. */
int xy_littlefs_w25q128_format_mount(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash);
/** Mount an explicitly bounded, block-aligned partition. */
int xy_littlefs_w25q128_mount_partition(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash,
                                        uint32_t base, uint32_t size, int format);
int xy_littlefs_w25q128_unmount(xy_littlefs_w25q128_t* volume);

#ifdef __cplusplus
}
#endif
#endif
