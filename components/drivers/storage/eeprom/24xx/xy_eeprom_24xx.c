/**
 * @file xy_eeprom_24xx.c
 * @brief 24xx Series EEPROM Device Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_eeprom_24xx.h"
#include <string.h>

#include "xy_hal_delay.h"

#define XY_EEPROM_24XX_MAX_PAGE_SIZE 126U

int xy_eeprom_24xx_init(xy_eeprom_24xx_t *eeprom, void *i2c_handle, 
                        uint16_t addr, uint16_t page_size, uint16_t total_size)
{
    int ret;

    if (!eeprom || !i2c_handle || page_size == 0 ||
        page_size > XY_EEPROM_24XX_MAX_PAGE_SIZE || total_size == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(eeprom, 0, sizeof(*eeprom));

    ret = xy_i2c_device_init(&eeprom->i2c_dev, i2c_handle, addr, 1000);
    if (ret != XY_DEVICE_OK) {
        memset(eeprom, 0, sizeof(*eeprom));
        return ret;
    }
    eeprom->page_size = page_size;
    eeprom->total_size = total_size;
    eeprom->address_bits = (total_size > 256) ? 16 : 8;

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

    /* Wait for write complete (max 5ms) */
    xy_hal_delay_ms(5);

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
