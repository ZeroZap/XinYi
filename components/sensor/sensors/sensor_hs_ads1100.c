#include "sensor_hs_ads1100.h"

#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t hs_ads1100_init(sensor_device_t *sensor)
{
    uint8_t ctrl = 0x57U;
    hs_ads1100_priv_t *priv = (hs_ads1100_priv_t *)sensor->priv_data;

    SENSOR_LOG("Initializing HS-ADS1100");
    if (hal_i2c_mem_write(sensor->bus, priv->i2c_addr, HS_ADS1100_REG_CTRL1, &ctrl, 1U)
        != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    return SENSOR_EOK;
}

static sensor_err_t hs_ads1100_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    hs_ads1100_priv_t *priv = (hs_ads1100_priv_t *)sensor->priv_data;

    for (uint8_t i = 0U; i < 6U; ++i) {
        if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, (uint8_t)(HS_ADS1100_REG_OUT_X_L + i),
                             &buf[i], 1U)
            != SENSOR_EOK) {
            return SENSOR_EIO;
        }
    }

    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int16_t)((buf[1] << 8) | buf[0]);
    data->value.val_3axis.y = (int16_t)((buf[3] << 8) | buf[2]);
    data->value.val_3axis.z = (int16_t)((buf[5] << 8) | buf[4]);
    data->timestamp = SENSOR_GET_TICK();

    return SENSOR_EOK;
}

static const sensor_ops_t hs_ads1100_ops = {.init = hs_ads1100_init, .read = hs_ads1100_read};

sensor_device_t *hs_ads1100_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    hs_ads1100_priv_t *priv = (hs_ads1100_priv_t *)SENSOR_MALLOC(sizeof(hs_ads1100_priv_t));

    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = (addr != 0U) ? addr : HS_ADS1100_ADDR_DEFAULT;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "Hangshun";
    sensor->info.model = "HS-ADS1100";
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->ops = &hs_ads1100_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;

    return sensor;
}
