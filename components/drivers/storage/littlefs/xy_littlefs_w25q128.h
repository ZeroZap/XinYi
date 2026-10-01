#ifndef XY_LITTLEFS_W25Q128_H
#define XY_LITTLEFS_W25Q128_H

#include "lfs.h"
#include "xy_w25q128.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lfs_t fs;
    struct lfs_config config;
    xy_w25q128_t* flash;
    uint8_t mounted;
} xy_littlefs_w25q128_t;

/** Mount a LittleFS volume on the complete W25Q128 device. */
int xy_littlefs_w25q128_mount(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash);

/** Format and mount a LittleFS volume. This destroys the selected volume. */
int xy_littlefs_w25q128_format_mount(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash);

/** Unmount a mounted LittleFS volume. */
int xy_littlefs_w25q128_unmount(xy_littlefs_w25q128_t* volume);

#ifdef __cplusplus
}
#endif

#endif
