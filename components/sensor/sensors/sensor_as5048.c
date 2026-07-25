#include "sensor_as5048.h"

#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);

static sensor_err_t as5048_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("Initializing AS5048");
    return SENSOR_EOK;
}

static sensor_err_t as5048_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || data == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }

    uint8_t buf[2];
    as5048_priv_t *priv = (as5048_priv_t *)sensor->priv_data;
    sensor_err_t ret = (sensor_err_t)hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0xFE, buf, 2);
    if (ret != SENSOR_EOK) {
        return ret;
    }

    data->type = SENSOR_TYPE_ANGLE;
    data->value.val_float = (((uint16_t)buf[1] << 8) | buf[0]) / 16384.0f * 360.0f;
    data->timestamp = SENSOR_GET_TICK();
    return SENSOR_EOK;
}

static const sensor_ops_t as5048_ops = {.init = as5048_init, .read = as5048_read};

sensor_device_t *as5048_create(const char *name, void *i2c_bus)
{
    if (name == NULL) {
        return NULL;
    }

    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    as5048_priv_t *priv = (as5048_priv_t *)SENSOR_MALLOC(sizeof(as5048_priv_t));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = AS5048_ADDR;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "AMS";
    sensor->info.model = "AS5048";
    sensor->info.type = SENSOR_TYPE_ANGLE;
    sensor->ops = &as5048_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    return sensor;
}
