/**
 * @file xy_sht30.c
 * @brief SHT30 Temperature/Humidity Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_sht30.h"
#include "xy_hal_delay.h"
#include <string.h>

/* SHT30 Commands */
#define SHT30_CMD_MEASURE       0x2C06
#define SHT30_CMD_SOFT_RESET    0x30A2

static uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU;

    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }

    return crc;
}

int xy_sht30_init(xy_sht30_t *sht, void *i2c_handle)
{
    int result;

    if (!sht || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(sht, 0, sizeof(*sht));
    result = xy_i2c_device_init(&sht->i2c_dev, i2c_handle, 0x44, 1000);
    if (result != XY_DEVICE_OK) {
        return result;
    }

    /* Soft reset */
    uint8_t reset_cmd[2] = {0x30, 0xA2};
    result = xy_i2c_device_write(&sht->i2c_dev, reset_cmd, 2);
    if (result < 0) {
        sht->i2c_dev.base.initialized = false;
        return result;
    }

    return XY_DEVICE_OK;
}

int xy_sht30_read(xy_sht30_t *sht)
{
    if (!sht || !sht->i2c_dev.base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* Send measure command */
    uint8_t measure_cmd[2] = {0x2C, 0x06};
    int result = xy_i2c_device_write(&sht->i2c_dev, measure_cmd, 2);
    if (result < 0) {
        return result;
    }
    
    /* Wait for measurement (15ms) */
    xy_hal_delay_ms(15);
    
    /* Read data (6 bytes) */
    uint8_t buffer[6];
    result = xy_i2c_device_read(&sht->i2c_dev, buffer, 6);
    if (result < 0) {
        return result;
    }

    if (sht30_crc8(buffer, 2U) != buffer[2] || sht30_crc8(&buffer[3], 2U) != buffer[5]) {
        return XY_DEVICE_IO_ERROR;
    }

    /* Parse temperature */
    uint16_t temp_raw = (buffer[0] << 8) | buffer[1];
    sht->temperature = (int16_t)((int32_t)temp_raw * 17500 / 65535 - 4500);
    
    /* Parse humidity */
    uint16_t hum_raw = (buffer[3] << 8) | buffer[4];
    sht->humidity = (uint16_t)((uint32_t)hum_raw * 10000 / 65535);
    
    return XY_DEVICE_OK;
}

int16_t xy_sht30_read_temperature(xy_sht30_t *sht)
{
    xy_sht30_read(sht);
    return sht->temperature;
}

uint16_t xy_sht30_read_humidity(xy_sht30_t *sht)
{
    xy_sht30_read(sht);
    return sht->humidity;
}
