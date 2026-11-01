/*
 * Host stand-in for the Device/I2C/HAL/OSAL backend behind the legacy `sensor_*`
 * samples in `components/sensor/example/`.
 *
 * The samples call real driver code (application -> `sensor_core` -> legacy
 * `sensor_<chip>` wrapper -> canonical `xy_<chip>` -> `xy_i2c_device_*` /
 * `xy_os_*`). This file supplies those backend entry points and answers exactly
 * one chip: register reads return the MPU6050 WHO_AM_I value so
 * `xy_mpu6050_init_addr()` completes, and the 14-byte burst at `ACCEL_XOUT_H`
 * returns a fixed sample, so the mg values the sample prints are computable from
 * the datasheet scaling (16384 LSB/g at +/-2g): 0x4000 -> +1000 mg,
 * 0xC000 -> -1000 mg, 0x2000 -> +500 mg.
 *
 * Anything else reads back as zero and every write succeeds. A sample that
 * starts talking to a different register or a different chip therefore changes
 * its printed numbers, and `sensor_example_basic`'s expected line stops
 * matching instead of passing against a rubber stamp.
 */

#include "xy_device.h"
#include "xy_hal_delay.h"
#include "xy_hal_sys.h"
#include "xy_os.h"

#include <string.h>

#define STUB_REG_WHO_AM_I 0x75U
#define STUB_WHO_AM_I_VALUE 0x68U
#define STUB_REG_ACCEL_XOUT_H 0x3BU
#define STUB_BURST_LEN 14U

/* Monotonic on purpose: the samples print `[timestamp]` and feeds it into
 * `sensor_data_t.timestamp`, so a frozen zero clock would make a stuck read
 * loop indistinguishable from a working one. */
static uint32_t stub_tick;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr,
                              uint32_t timeout)
{
    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    dev->base.initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t len)
{
    (void)reg;
    (void)data;
    (void)len;
    if (dev == NULL || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t len)
{
    if (dev == NULL || !dev->base.initialized || data == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(data, 0, len);
    if (reg == STUB_REG_WHO_AM_I && len >= 1U) {
        data[0] = STUB_WHO_AM_I_VALUE;
        return XY_DEVICE_OK;
    }
    if (reg == STUB_REG_ACCEL_XOUT_H && len >= STUB_BURST_LEN) {
        /* +1.0g, -1.0g, +0.5g, then gyro/temperature zeros. */
        data[0] = 0x40U;
        data[1] = 0x00U;
        data[2] = 0xC0U;
        data[3] = 0x00U;
        data[4] = 0x20U;
        data[5] = 0x00U;
    }
    return XY_DEVICE_OK;
}

/* Command-style transfers (AHT20 and friends): accepted, never answered. */
xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
    if (dev == NULL || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    if (dev == NULL || !dev->base.initialized || data == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(data, 0, len);
    return XY_DEVICE_OK;
}

void xy_hal_delay_ms(uint32_t ms)
{
    stub_tick += ms;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return stub_tick;
}

uint32_t xy_os_tick_get(void)
{
    return stub_tick;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    stub_tick += ticks;
    return XY_OS_OK;
}
