#include "xy_fota_w25q128.h"

#define SECTOR_SIZE 4096U
#define PAGE_SIZE 256U

typedef struct {
    xy_w25q128_t *flash;
    uint32_t base;
    uint32_t size;
    uint32_t timeout_ms;
} adapter_context_t;

static adapter_context_t context;

static int range_valid(uint32_t address, uint32_t size)
{
    return context.flash != NULL && size != 0U && address >= context.base &&
           address - context.base <= context.size &&
           size <= context.size - (address - context.base);
}

static int adapter_init(void)
{
    return context.flash != NULL && context.flash->initialized != 0U ? XY_FOTA_OK
                                                                      : XY_FOTA_FLASH_ERROR;
}

static int adapter_deinit(void)
{
    context.flash = NULL;
    context.base = 0U;
    context.size = 0U;
    context.timeout_ms = 0U;
    return XY_FOTA_OK;
}

static int adapter_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (data == NULL || !range_valid(address, size)) {
        return XY_FOTA_INVALID_PARAM;
    }
    return xy_w25q128_read(context.flash, address, data, size) == XY_W25Q128_OK
               ? XY_FOTA_OK
               : XY_FOTA_FLASH_ERROR;
}

static int adapter_write(uint32_t address, const uint8_t *data, uint32_t size)
{
    uint32_t offset = 0U;

    if (size == 0U) {
        return XY_FOTA_OK;
    }
    if (data == NULL || !range_valid(address, size)) {
        return XY_FOTA_INVALID_PARAM;
    }
    while (offset < size) {
        uint32_t page_remaining = PAGE_SIZE - ((address + offset) % PAGE_SIZE);
        uint32_t chunk = size - offset < page_remaining ? size - offset : page_remaining;
        if (xy_w25q128_page_program(context.flash, address + offset, data + offset, chunk,
                                    context.timeout_ms) != XY_W25Q128_OK) {
            return XY_FOTA_FLASH_ERROR;
        }
        offset += chunk;
    }
    return XY_FOTA_OK;
}

static int adapter_erase(uint32_t address, uint32_t size)
{
    uint32_t offset;

    if (!range_valid(address, size) || address % SECTOR_SIZE != 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    for (offset = 0U; offset < size; offset += SECTOR_SIZE) {
        if (xy_w25q128_sector_erase(context.flash, address + offset, context.timeout_ms) !=
            XY_W25Q128_OK) {
            return XY_FOTA_FLASH_ERROR;
        }
    }
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t ops = {
    .init = adapter_init,
    .write = adapter_write,
    .read = adapter_read,
    .erase = adapter_erase,
    .deinit = adapter_deinit,
};

int xy_fota_w25q128_bind(xy_w25q128_t *flash, uint32_t base, uint32_t size,
                          uint32_t timeout_ms)
{
    if (flash == NULL || flash->initialized == 0U || size == 0U || timeout_ms == 0U ||
        base % SECTOR_SIZE != 0U ||
        base > flash->capacity || size > flash->capacity - base) {
        return XY_FOTA_INVALID_PARAM;
    }
    context.flash = flash;
    context.base = base;
    context.size = size;
    context.timeout_ms = timeout_ms;
    return XY_FOTA_OK;
}

const xy_fota_flash_ops_t *xy_fota_w25q128_ops(void)
{
    return &ops;
}
