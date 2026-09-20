#include "sensor_icm20608.h"

#include <string.h>

extern int hal_spi_read_reg(void *bus, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_spi_write_reg(void *bus, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t icm20608_map_error(xy_error_t result)
{
    if (result == XY_DEVICE_OK) {
        return SENSOR_EOK;
    }
    if (result == XY_DEVICE_INVALID_PARAM) {
        return SENSOR_EINVAL;
    }
    if (result == XY_DEVICE_TIMEOUT || result == SENSOR_ETIMEOUT) {
        return SENSOR_ETIMEOUT;
    }
    if (result == XY_DEVICE_NOT_FOUND) {
        return SENSOR_ERROR;
    }
    return SENSOR_EIO;
}

static xy_error_t icm20608_spi_read(void *context, uint8_t reg, uint8_t *data, uint16_t len)
{
    return (xy_error_t)hal_spi_read_reg(context, reg, data, len);
}

static xy_error_t icm20608_spi_write(void *context, uint8_t reg, const uint8_t *data,
                                     uint16_t len)
{
    return (xy_error_t)hal_spi_write_reg(context, reg, (uint8_t *)data, len);
}

static sensor_err_t icm20608_init(sensor_device_t *sensor)
{
    icm20608_priv_t *priv;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (icm20608_priv_t *)sensor->priv_data;
    if (priv->use_spi) {
        result = xy_icm20608_init_spi(&priv->device, sensor->bus, icm20608_spi_read,
                                      icm20608_spi_write);
    } else {
        result = xy_icm20608_init_i2c(&priv->device, sensor->bus, priv->addr);
    }
    return icm20608_map_error(result);
}

static sensor_err_t icm20608_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(
        xy_icm20608_deinit(&((icm20608_priv_t *)sensor->priv_data)->device));
}

static sensor_err_t icm20608_accel_read(sensor_device_t *sensor, sensor_data_t *data)
{
    xy_icm20608_accel_t accel;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_icm20608_read_accel(&((icm20608_priv_t *)sensor->priv_data)->device, &accel);
    if (result != XY_DEVICE_OK) {
        return icm20608_map_error(result);
    }
    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = accel.x_mg;
    data->value.val_3axis.y = accel.y_mg;
    data->value.val_3axis.z = accel.z_mg;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95U;
    return SENSOR_EOK;
}

static sensor_err_t icm20608_gyro_read(sensor_device_t *sensor, sensor_data_t *data)
{
    xy_icm20608_gyro_t gyro;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_icm20608_read_gyro(&((icm20608_priv_t *)sensor->priv_data)->device, &gyro);
    if (result != XY_DEVICE_OK) {
        return icm20608_map_error(result);
    }
    data->type = SENSOR_TYPE_GYROSCOPE;
    data->unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = gyro.x_mdps / 1000;
    data->value.val_3axis.y = gyro.y_mdps / 1000;
    data->value.val_3axis.z = gyro.z_mdps / 1000;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95U;
    return SENSOR_EOK;
}

static sensor_err_t icm20608_temp_read(sensor_device_t *sensor, sensor_data_t *data)
{
    int32_t centi_c;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_icm20608_read_temperature(&((icm20608_priv_t *)sensor->priv_data)->device,
                                          &centi_c);
    if (result != XY_DEVICE_OK) {
        return icm20608_map_error(result);
    }
    data->type = SENSOR_TYPE_TEMPERATURE;
    data->unit = SENSOR_UNIT_CELSIUS;
#if SENSOR_USE_FLOAT
    data->value.val_float = (float)centi_c / 100.0F;
#else
    data->value.val_int32 = centi_c;
#endif
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 90U;
    return SENSOR_EOK;
}

static const sensor_ops_t icm20608_accel_ops = {
    .init = icm20608_init, .deinit = icm20608_deinit, .read = icm20608_accel_read,
};
static const sensor_ops_t icm20608_gyro_ops = {
    .init = icm20608_init, .deinit = icm20608_deinit, .read = icm20608_gyro_read,
};
static const sensor_ops_t icm20608_temp_ops = {
    .init = icm20608_init, .deinit = icm20608_deinit, .read = icm20608_temp_read,
};

static sensor_device_t *icm20608_create(const char *name, void *bus, bool use_spi,
                                        sensor_type_t type, const sensor_ops_t *ops)
{
    sensor_device_t *sensor;
    icm20608_priv_t *priv;

    if (name == NULL || bus == NULL) {
        return NULL;
    }
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    priv = (icm20608_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }
    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->addr = ICM20608_ADDR_DEFAULT;
    priv->use_spi = use_spi;
    priv->accel_range = 4U;
    priv->gyro_range = 500U;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.name[SENSOR_NAME_MAX_LEN - 1U] = '\0';
    sensor->info.vendor = "TDK InvenSense";
    sensor->info.model = "ICM20608";
    sensor->info.version = 0x0100U;
    sensor->info.type = type;
    sensor->info.resolution = 16U;
    sensor->info.max_odr = 1000U;
    if (type == SENSOR_TYPE_ACCELEROMETER) {
        sensor->info.unit = SENSOR_UNIT_MILLI_G;
        sensor->info.range_max = 4000;
        sensor->info.range_min = -4000;
        sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_FIFO_SUPPORT;
        sensor->odr = 100U;
    } else if (type == SENSOR_TYPE_GYROSCOPE) {
        sensor->info.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
        sensor->info.range_max = 500;
        sensor->info.range_min = -500;
        sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_FIFO_SUPPORT;
        sensor->odr = 100U;
    } else {
        sensor->info.unit = SENSOR_UNIT_CELSIUS;
        sensor->info.range_max = 85;
        sensor->info.range_min = -40;
        sensor->odr = 10U;
    }
    sensor->ops = ops;
    sensor->bus = bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    return sensor;
}

sensor_device_t *icm20608_create_accel(const char *name, void *bus, bool use_spi)
{
    return icm20608_create(name, bus, use_spi, SENSOR_TYPE_ACCELEROMETER,
                           &icm20608_accel_ops);
}

sensor_device_t *icm20608_create_gyro(const char *name, void *bus, bool use_spi)
{
    return icm20608_create(name, bus, use_spi, SENSOR_TYPE_GYROSCOPE, &icm20608_gyro_ops);
}

sensor_device_t *icm20608_create_temp(const char *name, void *bus, bool use_spi)
{
    return icm20608_create(name, bus, use_spi, SENSOR_TYPE_TEMPERATURE, &icm20608_temp_ops);
}
