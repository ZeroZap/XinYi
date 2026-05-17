/**
 * @file xy_hal_flash.c
 * @brief Flash HAL STM32F4 Implementation
 */

#include "../../inc/xy_hal_flash.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include "stm32f4xx_hal_flash_ex.h"
#include <string.h>

/* STM32F405/407 sector addresses and sizes */
#define FLASH_BASE_ADDR     0x08000000UL
#define FLASH_SECTOR_COUNT  12

typedef struct {
    uint32_t addr;
    uint32_t size;
    uint32_t sector_id;
} flash_sector_t;

static const flash_sector_t g_sectors[] = {
    { 0x08000000, 0x4000,  FLASH_SECTOR_0  },
    { 0x08004000, 0x4000,  FLASH_SECTOR_1  },
    { 0x08008000, 0x4000,  FLASH_SECTOR_2  },
    { 0x0800C000, 0x4000,  FLASH_SECTOR_3  },
    { 0x08010000, 0x10000, FLASH_SECTOR_4  },
    { 0x08020000, 0x20000, FLASH_SECTOR_5  },
    { 0x08040000, 0x20000, FLASH_SECTOR_6  },
    { 0x08060000, 0x20000, FLASH_SECTOR_7  },
    { 0x08080000, 0x20000, FLASH_SECTOR_8  },
    { 0x080A0000, 0x20000, FLASH_SECTOR_9  },
    { 0x080C0000, 0x20000, FLASH_SECTOR_10 },
    { 0x080E0000, 0x20000, FLASH_SECTOR_11 },
};

typedef struct {
    uint8_t initialized;
    uint8_t locked;
} flash_ctx_t;

static flash_ctx_t g_flash_ctx = { 0 };

xy_hal_error_t xy_hal_flash_init(void *flash)
{
    XY_UNUSED(flash);
    g_flash_ctx.initialized = 1;
    g_flash_ctx.locked      = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_deinit(void *flash)
{
    XY_UNUSED(flash);
    HAL_FLASH_Lock();
    g_flash_ctx.initialized = 0;
    g_flash_ctx.locked      = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_read(void *flash, uint32_t addr, uint8_t *data,
                                  uint32_t size)
{
    XY_UNUSED(flash);
    if (!data || size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    memcpy(data, (const void *)addr, size);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_write(void *flash, uint32_t addr,
                                   const uint8_t *data, uint32_t size)
{
    XY_UNUSED(flash);
    if (!data || size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_flash_ctx.locked) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Write in 8-byte (double-word) chunks for best throughput */
    size_t offset = 0;
    while (offset < size) {
        uint64_t word = 0xFFFFFFFFFFFFFFFFULL;
        size_t chunk  = (size - offset >= 8) ? 8 : (size - offset);
        memcpy(&word, data + offset, chunk);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                              addr + offset, word) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
        offset += 8;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_erase(void *flash, uint32_t addr, uint32_t size)
{
    XY_UNUSED(flash);
    XY_UNUSED(size);

    /* Find the sector containing addr */
    uint32_t sector_id = FLASH_SECTOR_0;
    for (size_t i = 0; i < FLASH_SECTOR_COUNT; i++) {
        if (addr >= g_sectors[i].addr &&
            addr < (g_sectors[i].addr + g_sectors[i].size)) {
            sector_id = g_sectors[i].sector_id;
            break;
        }
    }

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = sector_id,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };

    uint32_t sector_err = 0;
    if (HAL_FLASHEx_Erase(&erase, &sector_err) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_mass_erase(void *flash)
{
    XY_UNUSED(flash);

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_MASSERASE,
        .Banks        = FLASH_BANK_1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };

    uint32_t sector_err = 0;
    if (HAL_FLASHEx_Erase(&erase, &sector_err) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_lock(void *flash)
{
    XY_UNUSED(flash);
    HAL_FLASH_Lock();
    g_flash_ctx.locked = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_unlock(void *flash)
{
    XY_UNUSED(flash);
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    g_flash_ctx.locked = 0;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_set_read_protect(void *flash,
                                              xy_hal_flash_rdp_level_t level)
{
    XY_UNUSED(flash);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_get_read_protect(void *flash,
                                              xy_hal_flash_rdp_level_t *level)
{
    XY_UNUSED(flash);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info)
{
    XY_UNUSED(flash);
    if (!info) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    info->flash_size       = 1024 * 1024;   /* 1 MB typical */
    info->sector_count     = FLASH_SECTOR_COUNT;
    info->page_size        = 4096;          /* smallest sector */
    info->write_alignment  = 8;             /* double-word */
    info->sectors          = NULL;
    return XY_HAL_OK;
}

int xy_hal_flash_is_valid_address(void *flash, uint32_t addr)
{
    XY_UNUSED(flash);
    return (addr >= FLASH_BASE_ADDR && addr < (FLASH_BASE_ADDR + 1024 * 1024)) ? 1 : 0;
}

xy_hal_error_t xy_hal_flash_get_sector_info(void *flash, uint32_t sector_num,
                                             xy_hal_flash_sector_info_t *info)
{
    XY_UNUSED(flash);
    if (sector_num >= FLASH_SECTOR_COUNT || !info) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    info->start_addr   = g_sectors[sector_num].addr;
    info->size         = g_sectors[sector_num].size;
    info->is_protected = 0;
    return XY_HAL_OK;
}

#endif /* STM32_HAL_ENABLED */
