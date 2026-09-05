#include "pandora_fota_install_flash.h"

#include "stm32l4xx_hal.h"

#include <string.h>

static int range_valid(uint32_t address, uint32_t size)
{
    return address >= PANDORA_FOTA_APP_BASE && address <= PANDORA_FOTA_EXECUTION_LIMIT &&
           size <= PANDORA_FOTA_EXECUTION_LIMIT - address;
}

static int install_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (data == NULL || !range_valid(address, size)) {
        return XY_FOTA_INVALID_PARAM;
    }
    memcpy(data, (const void *)(uintptr_t)address, size);
    return XY_FOTA_OK;
}

static int install_write(uint32_t address, const uint8_t *data, uint32_t size)
{
    int result = XY_FOTA_OK;

    if (data == NULL || !range_valid(address, size) || (address & 7U) != 0U ||
        (size & 7U) != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    for (uint32_t offset = 0U; offset < size; offset += 8U) {
        uint64_t value;
        memcpy(&value, data + offset, sizeof(value));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + offset, value) != HAL_OK) {
            result = XY_FOTA_FLASH_ERROR;
            break;
        }
    }
    if (HAL_FLASH_Lock() != HAL_OK) {
        result = XY_FOTA_FLASH_ERROR;
    }
    return result;
}

static int install_erase(uint32_t address, uint32_t size)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    int result = XY_FOTA_OK;

    if (!range_valid(address, size) || size == 0U ||
        (address % PANDORA_FOTA_INSTALL_ERASE_SIZE) != 0U ||
        (size % PANDORA_FOTA_INSTALL_ERASE_SIZE) != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = address >= FLASH_BASE + FLASH_BANK_SIZE ? FLASH_BANK_2 : FLASH_BANK_1;
    erase.Page = erase.Banks == FLASH_BANK_2
                     ? (address - FLASH_BASE - FLASH_BANK_SIZE) / FLASH_PAGE_SIZE
                     : (address - FLASH_BASE) / FLASH_PAGE_SIZE;
    erase.NbPages = size / FLASH_PAGE_SIZE;
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        result = XY_FOTA_FLASH_ERROR;
    }
    if (HAL_FLASH_Lock() != HAL_OK) {
        result = XY_FOTA_FLASH_ERROR;
    }
    return result;
}

static const xy_fota_boot_install_ops_t install_ops = {
    .erase = install_erase,
    .write = install_write,
    .read = install_read,
    .program_granule = PANDORA_FOTA_INSTALL_PROGRAM_SIZE,
    .erase_granule = PANDORA_FOTA_INSTALL_ERASE_SIZE,
};

const xy_fota_boot_install_ops_t *pandora_fota_install_ops(void)
{
    return &install_ops;
}

int pandora_fota_application_vectors_valid(void)
{
    const uint32_t *vectors = (const uint32_t *)PANDORA_FOTA_APP_BASE;
    uint32_t reset = vectors[1] & ~1U;
    int stack_valid = ((vectors[0] & 7U) == 0U) &&
                      ((vectors[0] > 0x20000000U && vectors[0] <= 0x20018000U) ||
                       (vectors[0] > 0x10000000U && vectors[0] <= 0x10008000U));
    return stack_valid && (vectors[1] & 1U) != 0U && reset >= PANDORA_FOTA_APP_BASE &&
           reset < PANDORA_FOTA_EXECUTION_LIMIT;
}
