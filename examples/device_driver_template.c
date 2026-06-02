/**
 * @file device_driver_template.c
 * @brief Build-checked template for writing a XinYi xy_device_t driver.
 *
 * This example is intentionally small and self-contained: it implements a
 * byte-addressable RAM device through the public xy_device_t lifecycle API.
 * Driver authors can copy the structure below and replace the RAM backend with
 * UART/SPI/I2C/GPIO/HAL calls.
 */

#include "xy_device.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEMPLATE_STORAGE_SIZE 32U

typedef struct {
    uint8_t storage[TEMPLATE_STORAGE_SIZE];
    uint32_t open_count;
    uint32_t close_count;
    uint8_t configured_value;
} template_driver_data_t;

typedef struct {
    uint8_t fill_value;
} template_driver_config_t;

static template_driver_data_t g_template_data;
static const template_driver_config_t g_template_config = {
    .fill_value = 0xA5,
};

static xy_error_t template_init(xy_device_t *dev, const void *config)
{
    if (!dev || !dev->data || !config) {
        return XY_ERROR_INVALID_PARAM;
    }

    template_driver_data_t *data = (template_driver_data_t *)dev->data;
    const template_driver_config_t *cfg = (const template_driver_config_t *)config;

    memset(data->storage, cfg->fill_value, sizeof(data->storage));
    data->configured_value = cfg->fill_value;
    data->open_count = 0;
    data->close_count = 0;

    dev->initialized = 1;
    dev->state = XY_DEV_STATE_READY;
    return XY_OK;
}

static xy_error_t template_deinit(xy_device_t *dev)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }

    dev->initialized = 0;
    dev->state = XY_DEV_STATE_CLOSED;
    return XY_OK;
}

static xy_error_t template_open(xy_device_t *dev, uint32_t flags)
{
    (void)flags;

    if (!dev || !dev->data || !dev->initialized) {
        return XY_ERROR_NOT_READY;
    }

    template_driver_data_t *data = (template_driver_data_t *)dev->data;
    data->open_count++;
    return XY_OK;
}

static xy_error_t template_close(xy_device_t *dev)
{
    if (!dev || !dev->data) {
        return XY_ERROR_INVALID_PARAM;
    }

    template_driver_data_t *data = (template_driver_data_t *)dev->data;
    data->close_count++;
    return XY_OK;
}

static int32_t template_read(xy_device_t *dev, uint32_t pos, void *buf, size_t size)
{
    if (!dev || !dev->data || !buf) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (pos >= TEMPLATE_STORAGE_SIZE) {
        return 0;
    }

    template_driver_data_t *data = (template_driver_data_t *)dev->data;
    size_t available = TEMPLATE_STORAGE_SIZE - pos;
    size_t to_copy = (size < available) ? size : available;

    memcpy(buf, &data->storage[pos], to_copy);
    return (int32_t)to_copy;
}

static int32_t template_write(xy_device_t *dev, uint32_t pos, const void *buf, size_t size)
{
    if (!dev || !dev->data || !buf) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (pos >= TEMPLATE_STORAGE_SIZE) {
        return 0;
    }

    template_driver_data_t *data = (template_driver_data_t *)dev->data;
    size_t available = TEMPLATE_STORAGE_SIZE - pos;
    size_t to_copy = (size < available) ? size : available;

    memcpy(&data->storage[pos], buf, to_copy);
    return (int32_t)to_copy;
}

static xy_error_t template_control(xy_device_t *dev, uint32_t cmd, void *args)
{
    if (!dev || !dev->data) {
        return XY_ERROR_INVALID_PARAM;
    }

    template_driver_data_t *data = (template_driver_data_t *)dev->data;

    switch ((xy_dev_cmd_t)cmd) {
    case XY_DEV_CMD_RESET:
        memset(data->storage, data->configured_value, sizeof(data->storage));
        return XY_OK;
    case XY_DEV_CMD_GET_INFO:
        if (!args) {
            return XY_ERROR_INVALID_PARAM;
        }
        return xy_device_get_info(dev, (xy_dev_info_t *)args);
    default:
        return XY_ERROR_NOT_SUPPORTED;
    }
}

static const xy_dev_api_t g_template_api = {
    .init = template_init,
    .deinit = template_deinit,
    .open = template_open,
    .close = template_close,
    .read = template_read,
    .write = template_write,
    .control = template_control,
};

static xy_device_t g_template_device = {
    .name = "tmpl0",
    .type = XY_DEV_TYPE_MISC,
    .flags = XY_DEV_FLAG_RDWR,
    .state = XY_DEV_STATE_INIT,
    .api = &g_template_api,
    .config = &g_template_config,
    .data = &g_template_data,
};

int main(void)
{
    uint8_t rx[4] = {0};
    const uint8_t tx[4] = {1, 2, 3, 4};
    xy_dev_info_t info;

    assert(xy_device_init() == XY_OK);
    assert(g_template_api.init(&g_template_device, g_template_device.config) == XY_OK);
    assert(xy_device_register(&g_template_device) == XY_OK);
    assert(xy_device_exists("tmpl0") == 1);

    xy_device_t *dev = xy_device_open("tmpl0", XY_DEV_FLAG_RDWR);
    assert(dev == &g_template_device);
    assert(g_template_data.open_count == 1U);

    assert(xy_device_write(dev, 4, tx, sizeof(tx)) == (int32_t)sizeof(tx));
    assert(xy_device_read(dev, 4, rx, sizeof(rx)) == (int32_t)sizeof(rx));
    assert(memcmp(rx, tx, sizeof(rx)) == 0);

    assert(xy_device_control(dev, XY_DEV_CMD_GET_INFO, &info) == XY_OK);
    assert(info.name && strcmp(info.name, "tmpl0") == 0);
    assert(info.type == XY_DEV_TYPE_MISC);

    assert(xy_device_control(dev, XY_DEV_CMD_RESET, NULL) == XY_OK);
    memset(rx, 0, sizeof(rx));
    assert(xy_device_read(dev, 4, rx, sizeof(rx)) == (int32_t)sizeof(rx));
    assert(rx[0] == g_template_config.fill_value);

    assert(xy_device_close(dev) == XY_OK);
    assert(g_template_data.close_count == 1U);
    assert(g_template_api.deinit(&g_template_device) == XY_OK);

    puts("device_driver_template: OK");
    return 0;
}
