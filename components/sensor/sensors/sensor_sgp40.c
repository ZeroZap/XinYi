#include "sensor_sgp40.h"

#include <string.h>

static sensor_err_t sgp40_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("Initializing SGP40");
    return SENSOR_EOK;
}

static sensor_err_t sgp40_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    data->type = SENSOR_TYPE_GAS;
    data->value.val_float = 100.0f;
    data->timestamp = SENSOR_GET_TICK();
    return SENSOR_EOK;
}

static const sensor_ops_t sgp40_ops = {.init = sgp40_init, .read = sgp40_read};

sensor_device_t *sgp40_create(const char *name, void *i2c_bus)
{
    if (name == NULL) {
        return NULL;
    }

    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    sgp40_priv_t *priv = (sgp40_priv_t *)SENSOR_MALLOC(sizeof(sgp40_priv_t));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = SGP40_ADDR;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "Sensirion";
    sensor->info.model = "SGP40";
    sensor->info.type = SENSOR_TYPE_GAS;
    sensor->ops = &sgp40_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    return sensor;
}
