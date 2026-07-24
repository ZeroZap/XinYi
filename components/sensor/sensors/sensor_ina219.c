#include "sensor_ina219.h"

#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t ina219_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("Initializing INA219");
    return SENSOR_EOK;
}

static sensor_err_t ina219_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    uint8_t buf[2] = {0};
    ina219_priv_t *priv = (ina219_priv_t *)sensor->priv_data;
    int ret = hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0x01, buf, sizeof(buf));
    if (ret != SENSOR_EOK) {
        return (sensor_err_t)ret;
    }

    int16_t value = (int16_t)(((int16_t)buf[0] << 8) | buf[1]);
    data->type = SENSOR_TYPE_CURRENT;
    data->value.val_float = value * 0.1f;
    data->timestamp = SENSOR_GET_TICK();
    return SENSOR_EOK;
}

static const sensor_ops_t ina219_ops = {.init = ina219_init, .read = ina219_read};

sensor_device_t *ina219_create(const char *name, void *i2c_bus)
{
    if (name == NULL) {
        return NULL;
    }

    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    ina219_priv_t *priv = (ina219_priv_t *)SENSOR_MALLOC(sizeof(ina219_priv_t));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = INA219_ADDR;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "TI";
    sensor->info.model = "INA219";
    sensor->info.type = SENSOR_TYPE_CURRENT;
    sensor->ops = &ina219_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    return sensor;
}
