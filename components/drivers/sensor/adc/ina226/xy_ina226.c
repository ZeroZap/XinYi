#include "xy_ina226.h"
#include "xy_os.h"

#include <string.h>

#define INA226_CALIBRATION_NUMERATOR 5120000000ULL
#define INA226_CONFIG_DEFAULT_BASE 0x0527U

static int ina226_ready(const xy_ina_t *ina)
{
    return ina != NULL && ina->initialized != 0U && ina->i2c_dev.base.initialized &&
           ina->i2c_dev.i2c_handle != NULL;
}

static int ina226_address_valid(uint8_t addr)
{
    return addr >= INA226_ADDR_GND && addr <= INA226_ADDR_SCL;
}

static int ina226_config_valid(const xy_ina_config_t *config)
{
    uint64_t denominator;
    uint64_t calibration;

    if (config == NULL || config->shunt_resistor_uohm == 0U || config->current_lsb_ua == 0U ||
        config->avg_samples > XY_INA_AVG_1024) {
        return 0;
    }
    denominator = (uint64_t)config->current_lsb_ua * config->shunt_resistor_uohm;
    calibration = INA226_CALIBRATION_NUMERATOR / denominator;
    return calibration > 0U && calibration <= UINT16_MAX;
}

static int ina226_write_reg(xy_ina_t *ina, uint8_t reg, uint16_t value)
{
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    return xy_i2c_device_write_reg(&ina->i2c_dev, reg, data, sizeof(data));
}

static int ina226_read_reg(xy_ina_t *ina, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int result;

    result = xy_i2c_device_read_reg(&ina->i2c_dev, reg, data, sizeof(data));
    if (result == XY_DEVICE_OK) {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return result;
}

int xy_ina_init(xy_ina_t *ina, void *i2c_handle, uint8_t addr,
                const xy_ina_config_t *config)
{
    uint16_t mfg_id;
    uint16_t die_id;
    uint16_t config_reg;
    uint64_t denominator;
    int result;

    if (ina == NULL || i2c_handle == NULL || !ina226_address_valid(addr) ||
        !ina226_config_valid(config)) {
        return XY_INA_INVALID_PARAM;
    }

    memset(ina, 0, sizeof(*ina));
    result = xy_i2c_device_init(&ina->i2c_dev, i2c_handle, addr, 1000U);
    if (result != XY_DEVICE_OK) {
        memset(ina, 0, sizeof(*ina));
        return result;
    }
    ina->addr = addr;
    ina->config = *config;

    result = ina226_read_reg(ina, INA226_REG_MFG_ID, &mfg_id);
    if (result != XY_DEVICE_OK) {
        memset(ina, 0, sizeof(*ina));
        return result;
    }
    if (mfg_id != INA226_MFG_ID_VALUE) {
        memset(ina, 0, sizeof(*ina));
        return XY_INA_NOT_FOUND;
    }

    result = ina226_read_reg(ina, INA226_REG_DIE_ID, &die_id);
    if (result != XY_DEVICE_OK) {
        memset(ina, 0, sizeof(*ina));
        return result;
    }
    if ((die_id & 0xFFF0U) != INA226_DIE_ID_VALUE) {
        memset(ina, 0, sizeof(*ina));
        return XY_INA_NOT_FOUND;
    }

    denominator = (uint64_t)config->current_lsb_ua * config->shunt_resistor_uohm;
    ina->calib_value = (uint16_t)(INA226_CALIBRATION_NUMERATOR / denominator);
    result = ina226_write_reg(ina, INA226_REG_CALIB, ina->calib_value);
    if (result != XY_DEVICE_OK) {
        memset(ina, 0, sizeof(*ina));
        return result;
    }

    config_reg = (uint16_t)(INA226_CONFIG_DEFAULT_BASE | ((uint16_t)config->avg_samples << 9));
    result = ina226_write_reg(ina, INA226_REG_CONFIG, config_reg);
    if (result != XY_DEVICE_OK) {
        memset(ina, 0, sizeof(*ina));
        return result;
    }

    ina->initialized = 1U;
    return XY_INA_OK;
}

int xy_ina_deinit(xy_ina_t *ina)
{
    int result;

    if (!ina226_ready(ina)) {
        return XY_INA_INVALID_PARAM;
    }
    result = ina226_write_reg(ina, INA226_REG_CONFIG, 0U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    ina->initialized = 0U;
    ina->i2c_dev.base.initialized = 0U;
    ina->i2c_dev.i2c_handle = NULL;
    return XY_INA_OK;
}

int xy_ina_read(xy_ina_t *ina)
{
    uint16_t bus_raw;
    uint16_t shunt_raw;
    uint16_t current_raw;
    uint16_t power_raw;
    xy_ina_data_t next;
    int result;

    if (!ina226_ready(ina)) {
        return XY_INA_INVALID_PARAM;
    }
    result = ina226_read_reg(ina, INA226_REG_BUS_VOLT, &bus_raw);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    result = ina226_read_reg(ina, INA226_REG_SHUNT_VOLT, &shunt_raw);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    result = ina226_read_reg(ina, INA226_REG_CURRENT, &current_raw);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    result = ina226_read_reg(ina, INA226_REG_POWER, &power_raw);
    if (result != XY_DEVICE_OK) {
        return result;
    }

    next.voltage_mv = bus_raw * 1.25f;
    next.shunt_voltage_uv = (int16_t)shunt_raw * 2.5f;
    next.current_ma = (int16_t)current_raw * (ina->config.current_lsb_ua / 1000.0f);
    next.power_mw = power_raw * (25.0f * ina->config.current_lsb_ua / 1000.0f);
    next.timestamp = xy_os_tick_get();
    ina->data = next;
    return XY_INA_OK;
}

int xy_ina_get_voltage(xy_ina_t *ina, float *voltage_mv)
{
    int result;
    if (ina == NULL || voltage_mv == NULL) return XY_INA_INVALID_PARAM;
    result = xy_ina_read(ina);
    if (result == XY_INA_OK) *voltage_mv = ina->data.voltage_mv;
    return result;
}

int xy_ina_get_current(xy_ina_t *ina, float *current_ma)
{
    int result;
    if (ina == NULL || current_ma == NULL) return XY_INA_INVALID_PARAM;
    result = xy_ina_read(ina);
    if (result == XY_INA_OK) *current_ma = ina->data.current_ma;
    return result;
}

int xy_ina_get_power(xy_ina_t *ina, float *power_mw)
{
    int result;
    if (ina == NULL || power_mw == NULL) return XY_INA_INVALID_PARAM;
    result = xy_ina_read(ina);
    if (result == XY_INA_OK) *power_mw = ina->data.power_mw;
    return result;
}

int xy_ina_get_shunt_voltage(xy_ina_t *ina, float *voltage_uv)
{
    int result;
    if (ina == NULL || voltage_uv == NULL) return XY_INA_INVALID_PARAM;
    result = xy_ina_read(ina);
    if (result == XY_INA_OK) *voltage_uv = ina->data.shunt_voltage_uv;
    return result;
}

int xy_ina_enable_alert(xy_ina_t *ina, bool enable)
{
    if (!ina226_ready(ina)) {
        return XY_INA_INVALID_PARAM;
    }
    return ina226_write_reg(ina, INA226_REG_MASK_EN, enable ? 0x8000U : 0U);
}
