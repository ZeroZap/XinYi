#include "xy_mq7.h"
#include "xy_hal_sys.h"

static int mq7_ready(const xy_mq7_t *dev)
{
    return dev != NULL && dev->adc != NULL && dev->initialized;
}

xy_error_t xy_mq7_init(xy_mq7_t *dev, void *adc, uint8_t channel)
{
    if (dev == NULL || adc == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->adc = adc;
    dev->channel = channel;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_mq7_deinit(xy_mq7_t *dev)
{
    if (!mq7_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->adc = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_mq7_read(xy_mq7_t *dev, xy_mq7_sample_t *sample)
{
    int32_t raw;

    if (!mq7_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    raw = xy_adc_dev_read(dev->adc, dev->channel, XY_MQ7_DEFAULT_TIMEOUT_MS);
    if (raw < 0) {
        return XY_DEVICE_ERROR;
    }
    sample->raw = (uint32_t)raw;
    sample->timestamp = xy_hal_sys_get_tick_count();
    return XY_DEVICE_OK;
}
