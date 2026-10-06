/**
 * @file xy_eeprom_24xx.c
 * @brief 24xx Series EEPROM Device Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_eeprom_24xx.h"
#include <string.h>

#include "xy_hal_i2c.h"

#define XY_EEPROM_24XX_MAX_PAGE_SIZE 126U
#define XY_EEPROM_24XX_READY_TRIALS 5U
#define XY_EEPROM_24XX_READY_TIMEOUT_MS 1U

int xy_eeprom_24xx_init(xy_eeprom_24xx_t *eeprom, void *i2c_handle, 
                        uint16_t addr, uint16_t page_size, uint16_t total_size)
{
    xy_eeprom_24xx_t candidate;
    bool preserve_live_owner;
    int ret;

    if (!eeprom || !i2c_handle || page_size == 0 ||
        page_size > XY_EEPROM_24XX_MAX_PAGE_SIZE || total_size == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }

    preserve_live_owner = eeprom->i2c_dev.base.initialized == 1U &&
                          eeprom->i2c_dev.i2c_handle != NULL && eeprom->page_size != 0U &&
                          eeprom->total_size != 0U &&
                          (eeprom->address_bits == 8U || eeprom->address_bits == 16U);
    memset(&candidate, 0, sizeof(candidate));

    ret = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, addr, 1000U);
    if (ret != XY_DEVICE_OK || candidate.i2c_dev.base.initialized != 1U ||
        candidate.i2c_dev.i2c_handle == NULL) {
        if (!preserve_live_owner) {
            memset(eeprom, 0, sizeof(*eeprom));
        }
        return ret == XY_DEVICE_OK ? XY_DEVICE_INVALID_PARAM : ret;
    }
    candidate.page_size = page_size;
    candidate.total_size = total_size;
    candidate.address_bits = (total_size > 256U) ? 16U : 8U;
    *eeprom = candidate;

    return XY_DEVICE_OK;
}

int xy_eeprom_24xx_read(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                        uint8_t *data, size_t len)
{
    if (!eeprom || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if (addr + len > eeprom->total_size) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Send address */
    uint8_t addr_buf[2];
    int ret;
    if (eeprom->address_bits == 16) {
        addr_buf[0] = (addr >> 8) & 0xFF;
        addr_buf[1] = addr & 0xFF;
        ret = xy_i2c_device_write(&eeprom->i2c_dev, addr_buf, 2);
    } else {
        addr_buf[0] = addr & 0xFF;
        ret = xy_i2c_device_write(&eeprom->i2c_dev, addr_buf, 1);
    }
    if (ret < 0) {
        return ret;
    }

    /* Read data */
    return xy_i2c_device_read(&eeprom->i2c_dev, data, len);
}

int xy_eeprom_24xx_write_page(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                              const uint8_t *data, size_t len)
{
    if (!eeprom || !data || !eeprom->i2c_dev.base.initialized ||
        eeprom->page_size == 0U || eeprom->page_size > XY_EEPROM_24XX_MAX_PAGE_SIZE ||
        addr >= eeprom->total_size) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Limit the transaction to both the current page and configured capacity. */
    size_t page_offset = (size_t)addr % eeprom->page_size;
    size_t page_available = (size_t)eeprom->page_size - page_offset;
    size_t capacity_available = (size_t)eeprom->total_size - addr;
    if (len > page_available) {
        len = page_available;
    }
    if (len > capacity_available) {
        len = capacity_available;
    }

    /* Send address and page payload */
    uint8_t buffer[128];
    int ret;
    if (eeprom->address_bits == 16) {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = addr & 0xFF;
        memcpy(&buffer[2], data, len);
        ret = xy_i2c_device_write(&eeprom->i2c_dev, buffer, len + 2);
    } else {
        buffer[0] = addr & 0xFF;
        memcpy(&buffer[1], data, len);
        ret = xy_i2c_device_write(&eeprom->i2c_dev, buffer, len + 1);
    }
    if (ret < 0) {
        return ret;
    }

    ret = xy_hal_i2c_is_device_ready(eeprom->i2c_dev.i2c_handle, eeprom->i2c_dev.dev_addr,
                                     XY_EEPROM_24XX_READY_TRIALS,
                                     XY_EEPROM_24XX_READY_TIMEOUT_MS);
    if (ret != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    return (int)len;
}

int xy_eeprom_24xx_write(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                         const uint8_t *data, size_t len)
{
    if (!eeprom || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if ((size_t)addr + len > eeprom->total_size) {
        return XY_DEVICE_INVALID_PARAM;
    }

    int total_written = 0;

    while (len > 0) {
        int written = xy_eeprom_24xx_write_page(eeprom, addr, data, len);
        if (written < 0) {
            return written;
        }

        total_written += written;
        addr += written;
        data += written;
        len -= written;
    }

    return total_written;
}
