#include "xy_ina22x.h"
#include "xy_os.h"

#include <limits.h>

#define INA22X_SHUNT_CAL_NUMERATOR 13107200ULL

static int32_t sign_extend20(uint32_t value)
{
    value &= 0xFFFFFU;
    if ((value & 0x80000U) != 0U) {
        value |= 0xFFF00000U;
    }
    return (int32_t)value;
}

static int64_t sign_extend40(uint64_t value)
{
    value &= 0xFFFFFFFFFFULL;
    if ((value & 0x8000000000ULL) != 0ULL) {
        value |= 0xFFFFFF0000000000ULL;
    }
    return (int64_t)value;
}

static uint16_t be16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | data[1];
}

static uint32_t be24(const uint8_t *data)
{
    return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

static uint64_t be40(const uint8_t *data)
{
    return ((uint64_t)data[0] << 32) | ((uint64_t)data[1] << 24) |
           ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 8) | data[4];
}

int xy_ina22x_core_config_valid(const xy_ina22x_config_t *config, uint16_t *shunt_cal)
{
    uint64_t calibration;

    if (config == NULL || shunt_cal == NULL || config->shunt_resistor_uohm == 0U ||
        config->current_lsb_ua == 0U || config->shunt_range > XY_INA22X_SHUNT_RANGE_40_96_MV) {
        return XY_DEVICE_INVALID_PARAM;
    }

    calibration = INA22X_SHUNT_CAL_NUMERATOR * config->current_lsb_ua;
    calibration = calibration * config->shunt_resistor_uohm / 1000000000ULL;
    if (config->shunt_range == XY_INA22X_SHUNT_RANGE_40_96_MV) {
        calibration *= 4ULL;
    }
    if (calibration == 0ULL || calibration > 0x7FFFULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    *shunt_cal = (uint16_t)calibration;
    return XY_DEVICE_OK;
}

int xy_ina22x_core_configure(xy_ina22x_core_t *core)
{
    uint16_t config;
    int result;

    if (core == NULL || core->transport.write16 == NULL || core->transport.context == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    config = core->config.shunt_range == XY_INA22X_SHUNT_RANGE_40_96_MV ? 0x0010U : 0U;
    result = core->transport.write16(core->transport.context, XY_INA22X_REG_CONFIG, config);
    if (result != XY_DEVICE_OK) return result;
    result = core->transport.write16(core->transport.context, XY_INA22X_REG_ADC_CONFIG,
                                     core->config.adc_config);
    if (result != XY_DEVICE_OK) return result;
    return core->transport.write16(core->transport.context, XY_INA22X_REG_SHUNT_CAL,
                                   core->shunt_cal);
}

int xy_ina22x_core_read(xy_ina22x_core_t *core)
{
    uint8_t vshunt[3];
    uint8_t vbus[3];
    uint8_t temperature[2];
    uint8_t current[3];
    uint8_t power[3];
    uint8_t energy[5];
    uint8_t charge[5];
    xy_ina22x_sample_t next;
    int32_t shunt_raw;
    int32_t current_raw;
    int64_t charge_raw;
    uint32_t bus_raw;
    int result;

    if (core == NULL || core->initialized == 0U || core->transport.read == NULL ||
        core->transport.context == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
#define READ_OR_RETURN(reg, buffer)                                                   \
    do {                                                                              \
        result = core->transport.read(core->transport.context, (reg), (buffer),       \
                                      (uint8_t)sizeof(buffer));                        \
        if (result != XY_DEVICE_OK) return result;                                     \
    } while (0)
    READ_OR_RETURN(XY_INA22X_REG_VSHUNT, vshunt);
    READ_OR_RETURN(XY_INA22X_REG_VBUS, vbus);
    READ_OR_RETURN(XY_INA22X_REG_DIETEMP, temperature);
    READ_OR_RETURN(XY_INA22X_REG_CURRENT, current);
    READ_OR_RETURN(XY_INA22X_REG_POWER, power);
    READ_OR_RETURN(XY_INA22X_REG_ENERGY, energy);
    READ_OR_RETURN(XY_INA22X_REG_CHARGE, charge);
#undef READ_OR_RETURN

    shunt_raw = sign_extend20(be24(vshunt) >> 4U);
    bus_raw = be24(vbus) >> 4U;
    current_raw = sign_extend20(be24(current) >> 4U);
    charge_raw = sign_extend40(be40(charge));
    next.shunt_voltage_uv = shunt_raw *
        (core->config.shunt_range == XY_INA22X_SHUNT_RANGE_40_96_MV ? 0.078125 : 0.3125);
    next.bus_voltage_mv = bus_raw * 0.1953125;
    next.die_temperature_c = (int16_t)be16(temperature) * 0.0078125;
    next.current_ma = current_raw * (core->config.current_lsb_ua / 1000.0);
    next.power_mw = be24(power) * (3.2 * core->config.current_lsb_ua / 1000.0);
    next.energy_mj = be40(energy) * (51.2 * core->config.current_lsb_ua / 1000.0);
    next.charge_mc = charge_raw * (core->config.current_lsb_ua / 1000.0);
    next.timestamp = xy_os_tick_get();
    core->sample = next;
    return XY_DEVICE_OK;
}

int xy_ina22x_core_shutdown(xy_ina22x_core_t *core)
{
    if (core == NULL || core->initialized == 0U || core->transport.write16 == NULL ||
        core->transport.context == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return core->transport.write16(core->transport.context, XY_INA22X_REG_ADC_CONFIG, 0U);
}
