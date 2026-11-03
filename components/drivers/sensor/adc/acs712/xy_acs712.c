#include "xy_acs712.h"
#include "xy_hal_sys.h"

static int acs712_ready(const xy_acs712_t *dev)
{
    return dev != NULL && dev->adc != NULL && dev->initialized;
}

xy_error_t xy_acs712_init(xy_acs712_t *dev, void *adc, uint8_t channel)
{
    if (dev == NULL || adc == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->adc = adc;
    dev->channel = channel;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_acs712_deinit(xy_acs712_t *dev)
{
    if (!acs712_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->adc = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_acs712_read(xy_acs712_t *dev, xy_acs712_sample_t *sample)
{
    int32_t raw;
    float voltage_mv;
    float current_a;

    if (!acs712_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    raw = xy_adc_dev_read(dev->adc, dev->channel, XY_ACS712_DEFAULT_TIMEOUT_MS);
    if (raw < 0) {
        return XY_DEVICE_ERROR;
    }

    /* raw / max * supply gives the pin voltage; subtract the Vcc/2 quiescent point and divide by
     * sensitivity to get signed current in amperes. The formula is the one the legacy owner used,
     * kept verbatim so the migration does not change the number the caller sees. */
    voltage_mv = (float)raw / (float)XY_ACS712_ADC_MAX * (float)XY_ACS712_SUPPLY_MV;
    current_a = (voltage_mv - (float)XY_ACS712_SUPPLY_MV / 2.0f) /
                (float)XY_ACS712_SENSITIVITY_MV_PER_A;

    sample->current_ma = (int32_t)(current_a * 1000.0f);
    sample->timestamp = xy_hal_sys_get_tick_count();
    return XY_DEVICE_OK;
}
