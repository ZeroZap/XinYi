#include "xy_sc7a22h.h"
#include "xy_device_timing.h"
#include <string.h>

static xy_error_t rd(xy_sc7a22h_t *d, uint8_t r, uint8_t *p, size_t n) { return xy_i2c_device_read_reg(&d->i2c_dev, r, p, n); }
static xy_error_t wr(xy_sc7a22h_t *d, uint8_t r, uint8_t v) { return xy_i2c_device_write_reg(&d->i2c_dev, r, &v, 1U); }

xy_error_t xy_sc7a22h_init(xy_sc7a22h_t *d, void *h)
{
    uint8_t id, value; xy_error_t r;
    if (!d || !h) return XY_DEVICE_INVALID_PARAM;
    memset(d, 0, sizeof(*d));
    r = xy_i2c_device_init(&d->i2c_dev, h, XY_SC7A22H_ADDR, 100U);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    r = rd(d, XY_SC7A22H_REG_WHO_AM_I, &id, 1U);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    if (id != XY_SC7A22H_WHO_AM_I_VALUE) { memset(d, 0, sizeof(*d)); return XY_DEVICE_NOT_FOUND; }
    r = wr(d, XY_SC7A22H_REG_PWR_CTRL, XY_SC7A22H_ACC_ENABLE);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    xy_device_delay_ms(10U);
    r = wr(d, XY_SC7A22H_REG_ACC_CONF, XY_SC7A22H_DEMO_ACC_CONF);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    r = wr(d, XY_SC7A22H_REG_ACC_RANGE, XY_SC7A22H_DEMO_ACC_RANGE);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    r = wr(d, XY_SC7A22H_REG_COM_CFG, XY_SC7A22H_DEMO_COM_CFG);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    r = wr(d, XY_SC7A22H_REG_INT_CFG1, XY_SC7A22H_DEMO_INT_CFG1);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    r = wr(d, XY_SC7A22H_REG_HPF_LPF_CFG, XY_SC7A22H_DEMO_FILTER_CFG);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    xy_device_delay_ms(2U);
    r = rd(d, XY_SC7A22H_REG_COM_CFG, &value, 1U);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    d->com_cfg = value;
    r = rd(d, XY_SC7A22H_REG_ACC_CONF, &value, 1U);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    d->acc_conf = value;
    r = rd(d, XY_SC7A22H_REG_ACC_RANGE, &value, 1U);
    if (r != XY_DEVICE_OK) { memset(d, 0, sizeof(*d)); return r; }
    d->acc_range = value;
    d->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a22h_deinit(xy_sc7a22h_t *d)
{
    if (!d || !d->initialized || !d->i2c_dev.base.initialized) return XY_DEVICE_INVALID_PARAM;
    d->initialized = 0U; d->i2c_dev.base.initialized = 0U; return XY_DEVICE_OK;
}

xy_error_t xy_sc7a22h_read_config(xy_sc7a22h_t *d)
{
    uint8_t value;
    uint8_t next_com_cfg;
    uint8_t next_acc_conf;
    uint8_t next_acc_range;
    xy_error_t r;
    if (!d || !d->initialized || !d->i2c_dev.base.initialized) return XY_DEVICE_INVALID_PARAM;
    r = rd(d, XY_SC7A22H_REG_COM_CFG, &value, 1U);
    if (r != XY_DEVICE_OK) return r;
    next_com_cfg = value;
    r = rd(d, XY_SC7A22H_REG_ACC_CONF, &value, 1U);
    if (r != XY_DEVICE_OK) return r;
    next_acc_conf = value;
    r = rd(d, XY_SC7A22H_REG_ACC_RANGE, &value, 1U);
    if (r != XY_DEVICE_OK) return r;
    next_acc_range = value;
    d->com_cfg = next_com_cfg;
    d->acc_conf = next_acc_conf;
    d->acc_range = next_acc_range;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a22h_read_status(xy_sc7a22h_t *d, uint8_t *status)
{
    xy_error_t r;
    if (!d || !status || !d->initialized || !d->i2c_dev.base.initialized) return XY_DEVICE_INVALID_PARAM;
    r = rd(d, XY_SC7A22H_REG_DATA_STAT, status, 1U);
    if (r == XY_DEVICE_OK) d->data_status = *status;
    return r;
}

xy_error_t xy_sc7a22h_read(xy_sc7a22h_t *d, xy_sc7a22h_data_t *out)
{
    uint8_t b[6]; xy_sc7a22h_data_t next; xy_error_t r;
    if (!d || !out || !d->initialized || !d->i2c_dev.base.initialized) return XY_DEVICE_INVALID_PARAM;
    r = rd(d, XY_SC7A22H_REG_OUT_X_H, b, sizeof(b)); if (r != XY_DEVICE_OK) return r;
    next.x=(int16_t)(((uint16_t)b[0]<<8)|b[1]); next.y=(int16_t)(((uint16_t)b[2]<<8)|b[3]); next.z=(int16_t)(((uint16_t)b[4]<<8)|b[5]);
    d->data=next; *out=next; return XY_DEVICE_OK;
}

xy_error_t xy_sc7a22h_read_accel(xy_sc7a22h_t *d, xy_sc7a22h_accel_t *out)
{
    xy_sc7a22h_data_t raw; int32_t sensitivity; xy_error_t r;
    if (!d || !out || !d->initialized || !d->i2c_dev.base.initialized) return XY_DEVICE_INVALID_PARAM;
    r = xy_sc7a22h_read(d, &raw); if (r != XY_DEVICE_OK) return r;
    switch (d->acc_range & 0x03U) { case 0U: sensitivity=61; break; case 1U: sensitivity=122; break; case 2U: sensitivity=244; break; default: sensitivity=488; break; }
    out->x_mg=((int32_t)(raw.x >> 4) * sensitivity) / 1000;
    out->y_mg=((int32_t)(raw.y >> 4) * sensitivity) / 1000;
    out->z_mg=((int32_t)(raw.z >> 4) * sensitivity) / 1000;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a22h_set_acc_config(xy_sc7a22h_t *d, uint8_t config)
{
    xy_error_t r; if (!d || !d->initialized || !d->i2c_dev.base.initialized) return XY_DEVICE_INVALID_PARAM;
    r=wr(d,XY_SC7A22H_REG_ACC_CONF,config); if(r==XY_DEVICE_OK)d->acc_conf=config; return r;
}

xy_error_t xy_sc7a22h_set_acc_range(xy_sc7a22h_t *d, uint8_t range)
{
    xy_error_t r;
    if (!d || !d->initialized || !d->i2c_dev.base.initialized || range > 0x03U)
        return XY_DEVICE_INVALID_PARAM;
    r = wr(d, XY_SC7A22H_REG_ACC_RANGE, range);
    if (r == XY_DEVICE_OK) d->acc_range = range;
    return r;
}
