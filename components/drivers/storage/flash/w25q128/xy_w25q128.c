#include "xy_w25q128.h"

#include <string.h>

#define W25Q128_JEDEC_ID 0xEF4018U
#define W25Q128_CAPACITY (16U * 1024U * 1024U)
#define W25Q128_PAGE_SIZE 256U
#define W25Q128_SECTOR_SIZE 4096U

static xy_w25q128_status_t map_error(xy_hal_error_t error)
{
    if (error == XY_HAL_OK) {
        return XY_W25Q128_OK;
    }
    return error == XY_HAL_ERROR_TIMEOUT ? XY_W25Q128_TIMEOUT : XY_W25Q128_ERROR;
}

static xy_w25q128_status_t command(xy_w25q128_t *flash, uint8_t instruction,
                                    uint32_t address, uint8_t has_address,
                                    xy_hal_qspi_lines_t data_lines, uint8_t *data,
                                    size_t length, bool write, uint32_t timeout)
{
    xy_hal_qspi_command_t operation = {
        .instruction = instruction,
        .has_address = has_address,
        .address = address,
        .address_bits = 24U,
        .instruction_lines = XY_HAL_QSPI_LINES_1,
        .address_lines = has_address != 0U ? XY_HAL_QSPI_LINES_1 : XY_HAL_QSPI_LINES_NONE,
        .data_lines = data_lines,
        .data_length = length,
        .write = write,
    };

    return map_error(xy_hal_qspi_command(flash->qspi, &operation, data, timeout));
}

static xy_w25q128_status_t wait_ready(xy_w25q128_t *flash, uint32_t timeout_ms)
{
    uint32_t start = xy_hal_qspi_tick_ms();
    uint8_t status;
    xy_w25q128_status_t result;

    do {
        result = command(flash, 0x05U, 0U, 0U, XY_HAL_QSPI_LINES_1, &status, 1U, false,
                         timeout_ms);
        if (result != XY_W25Q128_OK) {
            return result;
        }
        if ((status & 0x01U) == 0U) {
            return XY_W25Q128_OK;
        }
    } while ((xy_hal_qspi_tick_ms() - start) < timeout_ms);
    return XY_W25Q128_TIMEOUT;
}

static xy_w25q128_status_t write_enable(xy_w25q128_t *flash, uint32_t timeout_ms)
{
    return command(flash, 0x06U, 0U, 0U, XY_HAL_QSPI_LINES_NONE, NULL, 0U, true,
                   timeout_ms);
}

xy_w25q128_status_t xy_w25q128_init(xy_w25q128_t *flash, void *qspi, const char *name)
{
    uint8_t id[3];

    if (flash == NULL || qspi == NULL || name == NULL) {
        return XY_W25Q128_INVALID_PARAM;
    }
    memset(flash, 0, sizeof(*flash));
    flash->qspi = qspi;
    if (command(flash, 0x9FU, 0U, 0U, XY_HAL_QSPI_LINES_1, id, sizeof(id), false, 100U) !=
        XY_W25Q128_OK) {
        return XY_W25Q128_ERROR;
    }
    flash->jedec_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
    if (flash->jedec_id != W25Q128_JEDEC_ID) {
        return XY_W25Q128_NOT_FOUND;
    }
    flash->capacity = W25Q128_CAPACITY;
    flash->device.name = name;
    flash->device.type = XY_DEV_TYPE_FLASH;
    flash->device.initialized = true;
    flash->initialized = 1U;
    return xy_device_register(&flash->device) == XY_OK ? XY_W25Q128_OK : XY_W25Q128_ERROR;
}

xy_w25q128_status_t xy_w25q128_read(xy_w25q128_t *flash, uint32_t address, uint8_t *data,
                                    size_t length)
{
    if (flash == NULL || data == NULL || length == 0U || flash->initialized == 0U ||
        address > flash->capacity || length > flash->capacity - address) {
        return XY_W25Q128_INVALID_PARAM;
    }
    return command(flash, 0x03U, address, 1U, XY_HAL_QSPI_LINES_1, data, length, false, 100U);
}

xy_w25q128_status_t xy_w25q128_quad_read(xy_w25q128_t *flash, uint32_t address, uint8_t *data,
                                         size_t length, uint8_t dummy_cycles)
{
    xy_hal_qspi_command_t operation = {
        .instruction = 0x6BU,
        .has_address = 1U,
        .address = address,
        .address_bits = 24U,
        .instruction_lines = XY_HAL_QSPI_LINES_1,
        .address_lines = XY_HAL_QSPI_LINES_1,
        .data_lines = XY_HAL_QSPI_LINES_4,
        .dummy_cycles = dummy_cycles,
        .data_length = length,
        .write = false,
    };
    if (flash == NULL || data == NULL || length == 0U || flash->initialized == 0U ||
        address > flash->capacity || length > flash->capacity - address) {
        return XY_W25Q128_INVALID_PARAM;
    }
    return map_error(xy_hal_qspi_command(flash->qspi, &operation, data, 100U));
}

xy_w25q128_status_t xy_w25q128_sector_erase(xy_w25q128_t *flash, uint32_t address,
                                            uint32_t timeout_ms)
{
    xy_w25q128_status_t result;
    if (flash == NULL || flash->initialized == 0U || address % W25Q128_SECTOR_SIZE != 0U ||
        address >= flash->capacity || timeout_ms == 0U) {
        return XY_W25Q128_INVALID_PARAM;
    }
    result = write_enable(flash, timeout_ms);
    if (result == XY_W25Q128_OK) {
        result = command(flash, 0x20U, address, 1U, XY_HAL_QSPI_LINES_NONE, NULL, 0U, true,
                         timeout_ms);
    }
    return result == XY_W25Q128_OK ? wait_ready(flash, timeout_ms) : result;
}

xy_w25q128_status_t xy_w25q128_page_program(xy_w25q128_t *flash, uint32_t address,
                                            const uint8_t *data, size_t length,
                                            uint32_t timeout_ms)
{
    xy_w25q128_status_t result = XY_W25Q128_ERROR;
    if (flash == NULL || data == NULL || length == 0U || length > W25Q128_PAGE_SIZE ||
        flash->initialized == 0U || address > flash->capacity ||
        length > flash->capacity - address || (address % W25Q128_PAGE_SIZE) + length > W25Q128_PAGE_SIZE ||
        timeout_ms == 0U) {
        return XY_W25Q128_INVALID_PARAM;
    }
    result = write_enable(flash, timeout_ms);
    if (result == XY_W25Q128_OK) {
        result = command(flash, 0x02U, address, 1U, XY_HAL_QSPI_LINES_1, (uint8_t *)data,
                         length, true, timeout_ms);
    }
    return result == XY_W25Q128_OK ? wait_ready(flash, timeout_ms) : result;
}

xy_w25q128_status_t xy_w25q128_quad_page_program(xy_w25q128_t *flash, uint32_t address,
                                                 const uint8_t *data, size_t length,
                                                 uint32_t timeout_ms)
{
    xy_w25q128_status_t result = XY_W25Q128_ERROR;
    if (flash == NULL || data == NULL || length == 0U || length > W25Q128_PAGE_SIZE ||
        flash->initialized == 0U || address > flash->capacity ||
        length > flash->capacity - address ||
        (address % W25Q128_PAGE_SIZE) + length > W25Q128_PAGE_SIZE || timeout_ms == 0U) {
        return XY_W25Q128_INVALID_PARAM;
    }
    result = write_enable(flash, timeout_ms);
    if (result == XY_W25Q128_OK) {
        result = command(flash, 0x32U, address, 1U, XY_HAL_QSPI_LINES_4, (uint8_t *)data,
                         length, true, timeout_ms);
    }
    return result == XY_W25Q128_OK ? wait_ready(flash, timeout_ms) : result;
}
