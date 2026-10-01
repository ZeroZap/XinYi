#include "xy_littlefs_w25q128.h"

#include <string.h>

#define XY_LFS_BLOCK_SIZE 4096U
#define XY_LFS_PAGE_SIZE 256U
#define XY_LFS_BLOCK_COUNT (16U * 1024U * 1024U / XY_LFS_BLOCK_SIZE)
#define XY_LFS_CACHE_SIZE XY_LFS_BLOCK_SIZE
#define XY_LFS_LOOKAHEAD_SIZE 128U
#define XY_LFS_TIMEOUT_MS 5000U

static int map_status(xy_w25q128_status_t status) {
    return status == XY_W25Q128_OK ? 0 : LFS_ERR_IO;
}

static int lfs_read(const struct lfs_config* config, lfs_block_t block, lfs_off_t off, void* buffer,
                    lfs_size_t size) {
    xy_littlefs_w25q128_t* volume = (xy_littlefs_w25q128_t*)config->context;
    uint32_t address;

    if (volume == NULL || volume->flash == NULL || buffer == NULL || off > XY_LFS_BLOCK_SIZE ||
        size > XY_LFS_BLOCK_SIZE - off || block >= XY_LFS_BLOCK_COUNT) {
        return LFS_ERR_INVAL;
    }
    address = block * XY_LFS_BLOCK_SIZE + off;
    return map_status(xy_w25q128_read(volume->flash, address, buffer, size));
}

static int lfs_prog(const struct lfs_config* config, lfs_block_t block, lfs_off_t off,
                    const void* buffer, lfs_size_t size) {
    xy_littlefs_w25q128_t* volume = (xy_littlefs_w25q128_t*)config->context;
    const uint8_t* data = (const uint8_t*)buffer;
    uint32_t address;

    if (volume == NULL || volume->flash == NULL || buffer == NULL || size == 0U ||
        off > XY_LFS_BLOCK_SIZE || size > XY_LFS_BLOCK_SIZE - off || block >= XY_LFS_BLOCK_COUNT ||
        (off % XY_LFS_PAGE_SIZE) != 0U || (size % XY_LFS_PAGE_SIZE) != 0U) {
        return LFS_ERR_INVAL;
    }
    address = block * XY_LFS_BLOCK_SIZE + off;
    while (size != 0U) {
        int result = map_status(xy_w25q128_page_program(volume->flash, address, data,
                                                        XY_LFS_PAGE_SIZE, XY_LFS_TIMEOUT_MS));
        if (result != 0) {
            return result;
        }
        address += XY_LFS_PAGE_SIZE;
        data += XY_LFS_PAGE_SIZE;
        size -= XY_LFS_PAGE_SIZE;
    }
    return 0;
}

static int lfs_erase(const struct lfs_config* config, lfs_block_t block) {
    xy_littlefs_w25q128_t* volume = (xy_littlefs_w25q128_t*)config->context;
    if (volume == NULL || volume->flash == NULL || block >= XY_LFS_BLOCK_COUNT) {
        return LFS_ERR_INVAL;
    }
    return map_status(
        xy_w25q128_sector_erase(volume->flash, block * XY_LFS_BLOCK_SIZE, XY_LFS_TIMEOUT_MS));
}

static int lfs_sync(const struct lfs_config* config) {
    return config != NULL && config->context != NULL ? 0 : LFS_ERR_INVAL;
}

static void configure(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash) {
    memset(volume, 0, sizeof(*volume));
    volume->flash = flash;
    volume->config.context = volume;
    volume->config.read = lfs_read;
    volume->config.prog = lfs_prog;
    volume->config.erase = lfs_erase;
    volume->config.sync = lfs_sync;
    volume->config.read_size = XY_LFS_PAGE_SIZE;
    volume->config.prog_size = XY_LFS_PAGE_SIZE;
    volume->config.block_size = XY_LFS_BLOCK_SIZE;
    volume->config.block_count = XY_LFS_BLOCK_COUNT;
    volume->config.block_cycles = 500;
    volume->config.cache_size = XY_LFS_CACHE_SIZE;
    volume->config.lookahead_size = XY_LFS_LOOKAHEAD_SIZE;
}

int xy_littlefs_w25q128_mount(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash) {
    int result;
    if (volume == NULL || flash == NULL || flash->initialized == 0U) {
        return LFS_ERR_INVAL;
    }
    configure(volume, flash);
    result = lfs_mount(&volume->fs, &volume->config);
    if (result == 0) {
        volume->mounted = 1U;
    }
    return result;
}

int xy_littlefs_w25q128_format_mount(xy_littlefs_w25q128_t* volume, xy_w25q128_t* flash) {
    int result;
    if (volume == NULL || flash == NULL || flash->initialized == 0U) {
        return LFS_ERR_INVAL;
    }
    configure(volume, flash);
    result = lfs_format(&volume->fs, &volume->config);
    if (result != 0) {
        return result;
    }
    result = lfs_mount(&volume->fs, &volume->config);
    if (result == 0) {
        volume->mounted = 1U;
    }
    return result;
}

int xy_littlefs_w25q128_unmount(xy_littlefs_w25q128_t* volume) {
    if (volume == NULL || volume->mounted == 0U) {
        return LFS_ERR_INVAL;
    }
    volume->mounted = 0U;
    return lfs_unmount(&volume->fs);
}
