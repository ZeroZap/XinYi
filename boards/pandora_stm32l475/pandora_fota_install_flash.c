#include "pandora_fota_install_flash.h"

#include "xy_hal_flash.h"

static int range_valid(uint32_t address, uint32_t size)
{
    return address >= PANDORA_FOTA_APP_BASE && address <= PANDORA_FOTA_EXECUTION_LIMIT &&
           size <= PANDORA_FOTA_EXECUTION_LIMIT - address;
}

static int flash_begin(void)
{
    xy_hal_error_t error = xy_hal_flash_init(NULL);
    if (error != XY_HAL_OK && error != XY_HAL_ERROR_ALREADY_INIT) {
        return XY_FOTA_FLASH_ERROR;
    }
    return xy_hal_flash_unlock(NULL) == XY_HAL_OK ? XY_FOTA_OK : XY_FOTA_FLASH_ERROR;
}

static int flash_end(int result)
{
    return xy_hal_flash_lock(NULL) == XY_HAL_OK ? result : XY_FOTA_FLASH_ERROR;
}

static int install_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (data == NULL || !range_valid(address, size)) {
        return XY_FOTA_INVALID_PARAM;
    }
    xy_hal_error_t error = xy_hal_flash_init(NULL);
    if (error != XY_HAL_OK && error != XY_HAL_ERROR_ALREADY_INIT) {
        return XY_FOTA_FLASH_ERROR;
    }
    return xy_hal_flash_read(NULL, address, data, size) == XY_HAL_OK ? XY_FOTA_OK
                                                                    : XY_FOTA_FLASH_ERROR;
}

static int install_write(uint32_t address, const uint8_t *data, uint32_t size)
{
    if (data == NULL || !range_valid(address, size) || (address & 7U) != 0U ||
        (size & 7U) != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (flash_begin() != XY_FOTA_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    return flash_end(xy_hal_flash_write(NULL, address, data, size) == XY_HAL_OK
                         ? XY_FOTA_OK
                         : XY_FOTA_FLASH_ERROR);
}

static int install_erase(uint32_t address, uint32_t size)
{
    if (!range_valid(address, size) || size == 0U ||
        (address % PANDORA_FOTA_INSTALL_ERASE_SIZE) != 0U ||
        (size % PANDORA_FOTA_INSTALL_ERASE_SIZE) != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (flash_begin() != XY_FOTA_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    return flash_end(xy_hal_flash_erase(NULL, address, size) == XY_HAL_OK
                         ? XY_FOTA_OK
                         : XY_FOTA_FLASH_ERROR);
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
