/**
 * @file sensor_ak09918.c
 * @brief AK09918 3轴磁力计驱动实现
 */
#include "sensor_ak09918.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t ak09918_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }

    uint8_t data;
    ak09918_priv_t *priv = (ak09918_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing AK09918");

    int ret = hal_i2c_mem_read(sensor->bus, priv->i2c_addr, AK09918_REG_WHOAMI, &data, 1);
    if (ret != SENSOR_EOK) return (sensor_err_t)ret;
    if (data != AK09918_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    ret = hal_i2c_mem_write(sensor->bus, priv->i2c_addr, AK09918_REG_CTRL2,
                            (uint8_t[]){0x08}, 1);
    if (ret != SENSOR_EOK) {
        return (sensor_err_t)ret;
    }
    SENSOR_DELAY_MS(50);
    ret = hal_i2c_mem_write(sensor->bus, priv->i2c_addr, AK09918_REG_CTRL1,
                            (uint8_t[]){0x1F}, 1);
    if (ret != SENSOR_EOK) {
        return (sensor_err_t)ret;
    }
    SENSOR_LOG("AK09918 initialized");
    return SENSOR_EOK;
}

static sensor_err_t ak09918_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }

    ak09918_priv_t *priv = (ak09918_priv_t *)sensor->priv_data;
    int ret = hal_i2c_mem_write(sensor->bus, priv->i2c_addr, AK09918_REG_CTRL1,
                                (uint8_t[]){0x00}, 1);
    if (ret != SENSOR_EOK) {
        return (sensor_err_t)ret;
    }
    return SENSOR_EOK;
}

static sensor_err_t ak09918_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    uint8_t buf[6];
    ak09918_priv_t *priv = (ak09918_priv_t *)sensor->priv_data;
    for (int i = 0; i < 6; i++) {
        int ret = hal_i2c_mem_read(sensor->bus, priv->i2c_addr, AK09918_REG_DATA + i, &buf[i], 1);
        if (ret != SENSOR_EOK) {
            return (sensor_err_t)ret;
        }
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    data->type = SENSOR_TYPE_MAGNETOMETER;
    data->unit = SENSOR_UNIT_MICRO_TESLA;
    data->value.val_3axis.x = (int32_t)raw[0] * 15;
    data->value.val_3axis.y = (int32_t)raw[1] * 15;
    data->value.val_3axis.z = (int32_t)raw[2] * 15;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 90;
    return SENSOR_EOK;
}

static const sensor_ops_t ak09918_ops = {
    .init = ak09918_init, .deinit = ak09918_deinit, .read = ak09918_read,
};

sensor_device_t *ak09918_create(const char *name, void *i2c_bus)
{
    if (name == NULL || i2c_bus == NULL) {
        return NULL;
    }

    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    ak09918_priv_t *priv = (ak09918_priv_t *)SENSOR_MALLOC(sizeof(ak09918_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ak09918_priv_t));
    priv->i2c_addr = AK09918_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "AKM";
    sensor->info.model = "AK09918";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_MAGNETOMETER;
    sensor->info.unit = SENSOR_UNIT_MICRO_TESLA;
    sensor->info.range_max = 4900;
    sensor->info.range_min = -4900;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 200;
    sensor->info.flags = 0;

    sensor->ops = &ak09918_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}
