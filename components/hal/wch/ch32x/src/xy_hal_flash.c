#include "xy_hal_flash.h"

#if defined(MCU_CH32) || defined(CH32V30x)

#include "ch32v30x_flash.h"

#include <string.h>

#define XY_CH32V307_FLASH_BASE 0x08000000U
#define XY_CH32V307_FLASH_SIZE (288U * 1024U)
#define XY_CH32V307_FLASH_END (XY_CH32V307_FLASH_BASE + XY_CH32V307_FLASH_SIZE)
#define XY_CH32V307_FLASH_PAGE_SIZE 4096U
#define XY_CH32V307_FLASH_WRITE_ALIGNMENT 4U
#define XY_CH32V307_FLASH_PAGE_COUNT (XY_CH32V307_FLASH_SIZE / XY_CH32V307_FLASH_PAGE_SIZE)

typedef struct {
    uint8_t initialized;
    uint8_t locked;
} xy_ch32v307_flash_context_t;

static xy_ch32v307_flash_context_t flash_context;

static int flash_range_valid(uint32_t address, uint32_t size) {
    return address >= XY_CH32V307_FLASH_BASE && address <= XY_CH32V307_FLASH_END &&
           size <= XY_CH32V307_FLASH_END - address;
}

xy_hal_error_t xy_hal_flash_init(void* flash) {
    XY_UNUSED(flash);
    if (flash_context.initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    flash_context.initialized = 1U;
    flash_context.locked = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_deinit(void* flash) {
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (flash_context.locked == 0U) {
        FLASH_Lock();
    }
    memset(&flash_context, 0, sizeof(flash_context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_read(void* flash, uint32_t address, uint8_t* data, uint32_t size) {
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (data == NULL || size == 0U || !flash_range_valid(address, size)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memcpy(data, (const void*)(uintptr_t)address, size);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_write(void* flash, uint32_t address, const uint8_t* data,
                                  uint32_t size) {
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U || flash_context.locked != 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (data == NULL || size == 0U || !flash_range_valid(address, size) ||
        (address % XY_CH32V307_FLASH_WRITE_ALIGNMENT) != 0U ||
        (size % XY_CH32V307_FLASH_WRITE_ALIGNMENT) != 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    for (uint32_t offset = 0U; offset < size; offset += XY_CH32V307_FLASH_WRITE_ALIGNMENT) {
        uint32_t value;
        memcpy(&value, data + offset, sizeof(value));
        if (FLASH_ProgramWord(address + offset, value) != FLASH_COMPLETE) {
            return XY_HAL_ERROR_FAIL;
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_erase(void* flash, uint32_t address, uint32_t size) {
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U || flash_context.locked != 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (size == 0U || !flash_range_valid(address, size) ||
        (address % XY_CH32V307_FLASH_PAGE_SIZE) != 0U ||
        (size % XY_CH32V307_FLASH_PAGE_SIZE) != 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    for (uint32_t offset = 0U; offset < size; offset += XY_CH32V307_FLASH_PAGE_SIZE) {
        if (FLASH_ErasePage(address + offset) != FLASH_COMPLETE) {
            return XY_HAL_ERROR_FAIL;
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_mass_erase(void* flash) {
    XY_UNUSED(flash);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_lock(void* flash) {
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    FLASH_Lock();
    flash_context.locked = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_unlock(void* flash) {
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    FLASH_Unlock();
    flash_context.locked = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_set_read_protect(void* flash, xy_hal_flash_rdp_level_t level) {
    XY_UNUSED(flash);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_get_read_protect(void* flash, xy_hal_flash_rdp_level_t* level) {
    XY_UNUSED(flash);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_get_info(void* flash, xy_hal_flash_info_t* info) {
    XY_UNUSED(flash);
    if (info == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    info->flash_size = XY_CH32V307_FLASH_SIZE;
    info->sector_count = XY_CH32V307_FLASH_PAGE_COUNT;
    info->page_size = XY_CH32V307_FLASH_PAGE_SIZE;
    info->write_alignment = XY_CH32V307_FLASH_WRITE_ALIGNMENT;
    info->sectors = NULL;
    return XY_HAL_OK;
}

int xy_hal_flash_is_valid_address(void* flash, uint32_t address) {
    XY_UNUSED(flash);
    return address >= XY_CH32V307_FLASH_BASE && address < XY_CH32V307_FLASH_END;
}

xy_hal_error_t xy_hal_flash_get_sector_info(void* flash, uint32_t sector_num,
                                            xy_hal_flash_sector_info_t* info) {
    XY_UNUSED(flash);
    if (info == NULL || sector_num >= XY_CH32V307_FLASH_PAGE_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    info->start_addr = XY_CH32V307_FLASH_BASE + sector_num * XY_CH32V307_FLASH_PAGE_SIZE;
    info->size = XY_CH32V307_FLASH_PAGE_SIZE;
    info->is_protected = 0U;
    return XY_HAL_OK;
}

#endif