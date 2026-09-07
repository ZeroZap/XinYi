/**
 * @file xy_hal_flash.c
 * @brief Flash HAL STM32L4 implementation
 */

#include "xy_hal_flash.h"

#if defined(STM32L4) || defined(STM32L4xx) || defined(STM32L475xx)

#include "stm32l4xx_hal.h"

#include <string.h>

#define XY_STM32L4_FLASH_BASE 0x08000000U
#define XY_STM32L4_FLASH_SIZE (1024U * 1024U)
#define XY_STM32L4_FLASH_END (XY_STM32L4_FLASH_BASE + XY_STM32L4_FLASH_SIZE)
#define XY_STM32L4_FLASH_WRITE_ALIGNMENT 8U
#define XY_STM32L4_FLASH_PAGE_COUNT (XY_STM32L4_FLASH_SIZE / FLASH_PAGE_SIZE)

typedef struct {
    uint8_t initialized;
    uint8_t locked;
} xy_stm32l4_flash_context_t;

static xy_stm32l4_flash_context_t flash_context;

static int flash_range_valid(uint32_t address, uint32_t size)
{
    return address >= XY_STM32L4_FLASH_BASE && address <= XY_STM32L4_FLASH_END &&
           size <= XY_STM32L4_FLASH_END - address;
}

xy_hal_error_t xy_hal_flash_init(void *flash)
{
    XY_UNUSED(flash);
    if (flash_context.initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    flash_context.initialized = 1U;
    flash_context.locked = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_deinit(void *flash)
{
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (flash_context.locked == 0U && HAL_FLASH_Lock() != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    flash_context.initialized = 0U;
    flash_context.locked = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_read(void *flash, uint32_t address, uint8_t *data, uint32_t size)
{
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (data == NULL || size == 0U || !flash_range_valid(address, size)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memcpy(data, (const void *)(uintptr_t)address, size);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_write(void *flash, uint32_t address, const uint8_t *data,
                                  uint32_t size)
{
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U || flash_context.locked != 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (data == NULL || size == 0U || !flash_range_valid(address, size) ||
        (address % XY_STM32L4_FLASH_WRITE_ALIGNMENT) != 0U ||
        (size % XY_STM32L4_FLASH_WRITE_ALIGNMENT) != 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    for (uint32_t offset = 0U; offset < size; offset += XY_STM32L4_FLASH_WRITE_ALIGNMENT) {
        uint64_t value;
        memcpy(&value, data + offset, sizeof(value));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + offset, value) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_erase(void *flash, uint32_t address, uint32_t size)
{
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U || flash_context.locked != 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (size == 0U || !flash_range_valid(address, size) ||
        (address % FLASH_PAGE_SIZE) != 0U || (size % FLASH_PAGE_SIZE) != 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    for (uint32_t offset = 0U; offset < size; offset += FLASH_PAGE_SIZE) {
        uint32_t page_address = address + offset;
        FLASH_EraseInitTypeDef erase = {0};
        uint32_t page_error = 0U;

        erase.TypeErase = FLASH_TYPEERASE_PAGES;
        erase.Banks = page_address >= XY_STM32L4_FLASH_BASE + FLASH_BANK_SIZE ? FLASH_BANK_2
                                                                             : FLASH_BANK_1;
        erase.Page = erase.Banks == FLASH_BANK_2
                         ? (page_address - XY_STM32L4_FLASH_BASE - FLASH_BANK_SIZE) /
                               FLASH_PAGE_SIZE
                         : (page_address - XY_STM32L4_FLASH_BASE) / FLASH_PAGE_SIZE;
        erase.NbPages = 1U;
        if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_mass_erase(void *flash)
{
    XY_UNUSED(flash);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_lock(void *flash)
{
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_FLASH_Lock() != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    flash_context.locked = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_unlock(void *flash)
{
    XY_UNUSED(flash);
    if (flash_context.initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    flash_context.locked = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_set_read_protect(void *flash, xy_hal_flash_rdp_level_t level)
{
    XY_UNUSED(flash);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_get_read_protect(void *flash, xy_hal_flash_rdp_level_t *level)
{
    XY_UNUSED(flash);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info)
{
    XY_UNUSED(flash);
    if (info == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    info->flash_size = XY_STM32L4_FLASH_SIZE;
    info->sector_count = XY_STM32L4_FLASH_PAGE_COUNT;
    info->page_size = FLASH_PAGE_SIZE;
    info->write_alignment = XY_STM32L4_FLASH_WRITE_ALIGNMENT;
    info->sectors = NULL;
    return XY_HAL_OK;
}

int xy_hal_flash_is_valid_address(void *flash, uint32_t address)
{
    XY_UNUSED(flash);
    return address >= XY_STM32L4_FLASH_BASE && address < XY_STM32L4_FLASH_END;
}

xy_hal_error_t xy_hal_flash_get_sector_info(void *flash, uint32_t sector_num,
                                            xy_hal_flash_sector_info_t *info)
{
    XY_UNUSED(flash);
    if (info == NULL || sector_num >= XY_STM32L4_FLASH_PAGE_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    info->start_addr = XY_STM32L4_FLASH_BASE + sector_num * FLASH_PAGE_SIZE;
    info->size = FLASH_PAGE_SIZE;
    info->is_protected = 0U;
    return XY_HAL_OK;
}

#endif
