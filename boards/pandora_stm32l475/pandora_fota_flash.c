#include "pandora_fota_flash.h"

#include "xy_hal_flash.h"

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

static int flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t end = PANDORA_FOTA_METADATA_BASE + 2U * PANDORA_FOTA_METADATA_ERASE_SIZE;

    if (!data || addr < PANDORA_FOTA_METADATA_BASE || addr > end || size > end - addr) {
        return XY_FOTA_INVALID_PARAM;
    }
    xy_hal_error_t error = xy_hal_flash_init(NULL);
    if (error != XY_HAL_OK && error != XY_HAL_ERROR_ALREADY_INIT) {
        return XY_FOTA_FLASH_ERROR;
    }
    return xy_hal_flash_read(NULL, addr, data, size) == XY_HAL_OK ? XY_FOTA_OK
                                                                 : XY_FOTA_FLASH_ERROR;
}

static int flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t end = PANDORA_FOTA_METADATA_BASE + 2U * PANDORA_FOTA_METADATA_ERASE_SIZE;

    if (!data || addr < PANDORA_FOTA_METADATA_BASE || addr > end || size > end - addr ||
        (addr & 7U) != 0U || (size & 7U) != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (flash_begin() != XY_FOTA_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    return flash_end(xy_hal_flash_write(NULL, addr, data, size) == XY_HAL_OK
                         ? XY_FOTA_OK
                         : XY_FOTA_FLASH_ERROR);
}

static int flash_erase(uint32_t addr, uint32_t size)
{
    if ((addr != PANDORA_FOTA_METADATA_BASE &&
         addr != PANDORA_FOTA_METADATA_BASE + PANDORA_FOTA_METADATA_ERASE_SIZE) ||
        size != PANDORA_FOTA_METADATA_ERASE_SIZE) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (flash_begin() != XY_FOTA_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    return flash_end(xy_hal_flash_erase(NULL, addr, size) == XY_HAL_OK
                         ? XY_FOTA_OK
                         : XY_FOTA_FLASH_ERROR);
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

xy_fota_boot_journal_config_t pandora_fota_boot_journal_config(void)
{
    xy_fota_boot_journal_config_t config = {
        .address = PANDORA_FOTA_METADATA_BASE,
        .slot_size = PANDORA_FOTA_METADATA_ERASE_SIZE,
        .read = flash_read,
        .erase = flash_erase,
        .write = flash_write,
    };
    return config;
}
