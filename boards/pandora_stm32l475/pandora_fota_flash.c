#include "pandora_fota_flash.h"

#include "stm32l4xx_hal.h"

#include <string.h>

static int flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t end = PANDORA_FOTA_METADATA_BASE + 2U * PANDORA_FOTA_METADATA_ERASE_SIZE;

    if (!data || addr < PANDORA_FOTA_METADATA_BASE || addr > end || size > end - addr) {
        return XY_FOTA_INVALID_PARAM;
    }
    memcpy(data, (const void *)(uintptr_t)addr, size);
    return XY_FOTA_OK;
}

static int flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t end = PANDORA_FOTA_METADATA_BASE + 2U * PANDORA_FOTA_METADATA_ERASE_SIZE;
    uint32_t offset = 0U;
    int ret = XY_FOTA_OK;

    if (!data || addr < PANDORA_FOTA_METADATA_BASE || addr > end || size > end - addr ||
        (addr & 7U) != 0U || (size & 7U) != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    while (offset < size) {
        uint64_t value = UINT64_MAX;
        uint32_t chunk = size - offset;
        if (chunk > sizeof(value)) {
            chunk = sizeof(value);
        }
        memcpy(&value, data + offset, chunk);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + offset, value) != HAL_OK) {
            ret = XY_FOTA_FLASH_ERROR;
            break;
        }
        offset += sizeof(value);
    }
    if (HAL_FLASH_Lock() != HAL_OK) {
        ret = XY_FOTA_FLASH_ERROR;
    }
    return ret;
}

static int flash_erase(uint32_t addr, uint32_t size)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    int ret = XY_FOTA_OK;

    if ((addr != PANDORA_FOTA_METADATA_BASE &&
         addr != PANDORA_FOTA_METADATA_BASE + PANDORA_FOTA_METADATA_ERASE_SIZE) ||
        size != PANDORA_FOTA_METADATA_ERASE_SIZE) {
        return XY_FOTA_INVALID_PARAM;
    }
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
    erase.NbPages = 1U;
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        ret = XY_FOTA_FLASH_ERROR;
    }
    if (HAL_FLASH_Lock() != HAL_OK) {
        ret = XY_FOTA_FLASH_ERROR;
    }
    return ret;
}

static const xy_fota_flash_ops_t flash_ops = {
    .write = flash_write,
    .read = flash_read,
    .erase = flash_erase,
};

static const xy_fota_metadata_flash_t metadata_backend = {
    .ops = &flash_ops,
    .base_addr = PANDORA_FOTA_METADATA_BASE,
    .erase_size = PANDORA_FOTA_METADATA_ERASE_SIZE,
};

const xy_fota_metadata_flash_t *pandora_fota_metadata_backend(void)
{
    return &metadata_backend;
}
