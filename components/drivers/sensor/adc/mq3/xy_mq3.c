#include "xy_mq3.h"
#include "xy_hal_sys.h"

static int mq3_ready(const xy_mq3_t *dev)
{
    return dev != NULL && dev->adc != NULL && dev->initialized;
}

xy_error_t xy_mq3_init(xy_mq3_t *dev, void *adc, uint8_t channel)
{
    if (dev == NULL || adc == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->adc = adc;
    dev->channel = channel;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_mq3_deinit(xy_mq3_t *dev)
{
    if (!mq3_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->adc = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_mq3_read(xy_mq3_t *dev, xy_mq3_sample_t *sample)
{
    int32_t raw;

    if (!mq3_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    raw = xy_adc_dev_read(dev->adc, dev->channel, XY_MQ3_DEFAULT_TIMEOUT_MS);
    if (raw < 0) {
        return XY_DEVICE_ERROR;
    }
    sample->raw = (uint32_t)raw;
    sample->timestamp = xy_hal_sys_get_tick_count();
    return XY_DEVICE_OK;
}
