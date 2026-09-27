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

#if SENSOR_ENABLE_POWER_MGMT
static sensor_err_t icm20608_set_power_mode(sensor_device_t *sensor, sensor_power_mode_t mode)
{
    uint8_t sleep;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    switch (mode) {
    case SENSOR_POWER_MODE_SHUTDOWN:
    case SENSOR_POWER_MODE_SLEEP:
    case SENSOR_POWER_MODE_STANDBY:
        sleep = 1U;
        break;
    case SENSOR_POWER_MODE_LOW_POWER:
    case SENSOR_POWER_MODE_NORMAL:
    case SENSOR_POWER_MODE_HIGH_PERFORMANCE:
        sleep = 0U;
        break;
    default:
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(
        xy_icm20608_set_sleep(&((icm20608_priv_t *)sensor->priv_data)->device, sleep));
}
#endif

#if SENSOR_ENABLE_INTERRUPT
static sensor_err_t icm20608_interrupt_enable(sensor_device_t *sensor, uint32_t int_type,
                                               bool enable)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        int_type != SENSOR_INT_DATA_READY) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(xy_icm20608_set_data_ready_interrupt(
        &((icm20608_priv_t *)sensor->priv_data)->device, enable ? 1U : 0U));
}
#endif

static const sensor_ops_t icm20608_accel_ops = {
    .init = icm20608_init, .deinit = icm20608_deinit, .read = icm20608_accel_read,
#if SENSOR_ENABLE_POWER_MGMT
    .set_power_mode = icm20608_set_power_mode,
#endif
#if SENSOR_ENABLE_INTERRUPT
    .interrupt_enable = icm20608_interrupt_enable,
#endif
};
static const sensor_ops_t icm20608_gyro_ops = {
    .init = icm20608_init, .deinit = icm20608_deinit, .read = icm20608_gyro_read,
#if SENSOR_ENABLE_POWER_MGMT
    .set_power_mode = icm20608_set_power_mode,
#endif
#if SENSOR_ENABLE_INTERRUPT
    .interrupt_enable = icm20608_interrupt_enable,
#endif
};
static const sensor_ops_t icm20608_temp_ops = {
    .init = icm20608_init, .deinit = icm20608_deinit, .read = icm20608_temp_read,
#if SENSOR_ENABLE_POWER_MGMT
    .set_power_mode = icm20608_set_power_mode,
#endif
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
        sensor->info.flags = SENSOR_FLAG_INT_SUPPORT;
        sensor->odr = 100U;
    } else if (type == SENSOR_TYPE_GYROSCOPE) {
        sensor->info.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
        sensor->info.range_max = 500;
        sensor->info.range_min = -500;
        sensor->info.flags = SENSOR_FLAG_INT_SUPPORT;
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

sensor_err_t icm20608_set_accel_range(sensor_device_t *sensor,
                                      xy_icm20608_accel_range_t range)
{
    static const int32_t range_mg[] = {2000, 4000, 8000, 16000};
    icm20608_priv_t *priv;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        sensor->info.type != SENSOR_TYPE_ACCELEROMETER || range > XY_ICM20608_ACCEL_RANGE_16G) {
        return SENSOR_EINVAL;
    }
    priv = (icm20608_priv_t *)sensor->priv_data;
    result = xy_icm20608_set_accel_range(&priv->device, range);
    if (result != XY_DEVICE_OK) return icm20608_map_error(result);
    priv->accel_range = (uint8_t)(range_mg[range] / 1000);
    sensor->info.range_min = -range_mg[range];
    sensor->info.range_max = range_mg[range];
    return SENSOR_EOK;
}

sensor_err_t icm20608_set_gyro_range(sensor_device_t *sensor,
                                     xy_icm20608_gyro_range_t range)
{
    static const int32_t range_dps[] = {250, 500, 1000, 2000};
    icm20608_priv_t *priv;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        sensor->info.type != SENSOR_TYPE_GYROSCOPE || range > XY_ICM20608_GYRO_RANGE_2000DPS) {
        return SENSOR_EINVAL;
    }
    priv = (icm20608_priv_t *)sensor->priv_data;
    result = xy_icm20608_set_gyro_range(&priv->device, range);
    if (result != XY_DEVICE_OK) return icm20608_map_error(result);
    priv->gyro_range = (uint16_t)range_dps[range];
    sensor->info.range_min = -range_dps[range];
    sensor->info.range_max = range_dps[range];
    return SENSOR_EOK;
}

sensor_err_t icm20608_set_odr(sensor_device_t *sensor, uint16_t odr_hz)
{
    icm20608_priv_t *priv;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        (sensor->info.type != SENSOR_TYPE_ACCELEROMETER &&
         sensor->info.type != SENSOR_TYPE_GYROSCOPE)) {
        return SENSOR_EINVAL;
    }
    priv = (icm20608_priv_t *)sensor->priv_data;
    result = xy_icm20608_set_odr(&priv->device, odr_hz);
    if (result != XY_DEVICE_OK) return icm20608_map_error(result);
    sensor->odr = odr_hz;
    return SENSOR_EOK;
}

sensor_err_t icm20608_set_dlpf(sensor_device_t *sensor, xy_icm20608_dlpf_t gyro_dlpf,
                               xy_icm20608_dlpf_t accel_dlpf)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        (sensor->info.type != SENSOR_TYPE_ACCELEROMETER &&
         sensor->info.type != SENSOR_TYPE_GYROSCOPE)) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(xy_icm20608_set_dlpf(
        &((icm20608_priv_t *)sensor->priv_data)->device, gyro_dlpf, accel_dlpf));
}

sensor_err_t icm20608_set_bias(sensor_device_t *sensor,
                               const xy_icm20608_accel_t *accel_bias,
                               const xy_icm20608_gyro_t *gyro_bias)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        accel_bias == NULL || gyro_bias == NULL ||
        (sensor->info.type != SENSOR_TYPE_ACCELEROMETER &&
         sensor->info.type != SENSOR_TYPE_GYROSCOPE)) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(xy_icm20608_set_bias(
        &((icm20608_priv_t *)sensor->priv_data)->device, accel_bias, gyro_bias));
}

sensor_err_t icm20608_get_bias(const sensor_device_t *sensor,
                               xy_icm20608_accel_t *accel_bias,
                               xy_icm20608_gyro_t *gyro_bias)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        accel_bias == NULL || gyro_bias == NULL ||
        (sensor->info.type != SENSOR_TYPE_ACCELEROMETER &&
         sensor->info.type != SENSOR_TYPE_GYROSCOPE)) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(xy_icm20608_get_bias(
        &((const icm20608_priv_t *)sensor->priv_data)->device, accel_bias, gyro_bias));
}

sensor_err_t icm20608_get_configuration(const sensor_device_t *sensor,
                                        xy_icm20608_configuration_t *configuration)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL ||
        configuration == NULL ||
        (sensor->info.type != SENSOR_TYPE_ACCELEROMETER &&
         sensor->info.type != SENSOR_TYPE_GYROSCOPE)) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(xy_icm20608_get_configuration(
        &((const icm20608_priv_t *)sensor->priv_data)->device, configuration));
}

sensor_err_t icm20608_read_interrupt_status(sensor_device_t *sensor, uint8_t *status)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || status == NULL ||
        (sensor->info.type != SENSOR_TYPE_ACCELEROMETER &&
         sensor->info.type != SENSOR_TYPE_GYROSCOPE)) {
        return SENSOR_EINVAL;
    }
    return icm20608_map_error(xy_icm20608_read_interrupt_status(
        &((icm20608_priv_t *)sensor->priv_data)->device, status));
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
