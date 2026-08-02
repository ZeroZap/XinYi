#include "sensor_bh1750.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t *data, uint16_t len);
static sensor_err_t bh1750_init(sensor_device_t *s)
{
    if (s == NULL || s->priv_data == NULL) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("Initializing BH1750");

    bh1750_priv_t *p = (bh1750_priv_t *)s->priv_data;
    uint8_t power_on_cmd = 0x01U;
    uint8_t continuous_h_res_cmd = 0x10U;

    if (hal_i2c_mem_write(s->bus, p->i2c_addr, &power_on_cmd, 1) != SENSOR_EOK) {
        return SENSOR_EIO;
    }
    if (hal_i2c_mem_write(s->bus, p->i2c_addr, &continuous_h_res_cmd, 1) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    return SENSOR_EOK;
}

static sensor_err_t bh1750_read(sensor_device_t *s, sensor_data_t *d)
{
    if (s == NULL || s->priv_data == NULL || d == NULL) {
        return SENSOR_EINVAL;
    }

    uint8_t buf[2];
    bh1750_priv_t *p = (bh1750_priv_t *)s->priv_data;
    uint8_t one_time_h_res_cmd = 0x20U;

    if (hal_i2c_mem_write(s->bus, p->i2c_addr, &one_time_h_res_cmd, 1) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    SENSOR_DELAY_MS(20);

    if (hal_i2c_mem_read(s->bus, p->i2c_addr, buf, 2) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    d->type = SENSOR_TYPE_LIGHT;
    d->unit = SENSOR_UNIT_LUX;
    d->value.val_float = (((uint16_t)buf[0] << 8) | buf[1]) / 1.2f;
    d->timestamp = SENSOR_GET_TICK();
    return SENSOR_EOK;
}

static const sensor_ops_t bh1750_ops = {.init = bh1750_init, .read = bh1750_read};

sensor_device_t *bh1750_create(const char *name, void *i2c_bus)
{
    if (name == NULL) {
        return NULL;
    }

    sensor_device_t *s = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    bh1750_priv_t *p = (bh1750_priv_t *)SENSOR_MALLOC(sizeof(bh1750_priv_t));
    if (s == NULL || p == NULL) {
        SENSOR_FREE(s);
        SENSOR_FREE(p);
        return NULL;
    }

    memset(s, 0, sizeof(sensor_device_t));
    p->i2c_addr = BH1750_ADDR;
    strncpy(s->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    s->info.vendor = "ROHM";
    s->info.model = "BH1750";
    s->info.type = SENSOR_TYPE_LIGHT;
    s->ops = &bh1750_ops;
    s->bus = i2c_bus;
    s->priv_data = p;
    s->status = SENSOR_STATUS_IDLE;
    return s;
}
