/**
 * @file xy_device.c
 * @brief Device lifecycle API. Registry storage lives in xy_device_core.c.
 *
 * This file provides the user-facing lifecycle operations (open/close/
 * read/write/control/async) and bus/sensor adapters. All registry queries
 * (register, unregister, find, enumerate) forward to the single static-
 * array registry implemented in xy_device_core.c so the framework has
 * exactly one source of truth for "what devices exist".
 */

#include "xy_device.h"
#include "xy_device_core.h"
#include <string.h>

/* ==================== Registry forwarding ==================== */

xy_error_t xy_device_init(void)
{
    return xy_device_registry_init();
}

xy_error_t xy_device_register(xy_device_t *dev)
{
    return xy_device_registry_register(dev);
}

xy_error_t xy_device_unregister(xy_device_t *dev)
{
    return xy_device_registry_unregister(dev);
}

xy_device_t *xy_device_find(const char *name)
{
    return xy_device_find_by_name(name);
}

/* ==================== Lifecycle ==================== */

xy_device_t *xy_device_open(const char *name, uint32_t flags)
{
    xy_device_t *dev = xy_device_find(name);
    if (!dev) {
        return NULL;
    }

    if (dev->api && dev->api->open) {
        if (dev->api->open(dev, flags) != XY_OK) {
            return NULL;
        }
    }

    dev->state = XY_DEV_STATE_OPENED;
    dev->ref_count++;
    return dev;
}

xy_error_t xy_device_close(xy_device_t *dev)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (dev->ref_count > 0) {
        dev->ref_count--;
    }

    if (dev->api && dev->api->close) {
        xy_error_t ret = dev->api->close(dev);
        if (ret != XY_OK) {
            return ret;
        }
    }

    dev->state = XY_DEV_STATE_CLOSED;
    return XY_OK;
}

int32_t xy_device_read(xy_device_t *dev, uint32_t pos, void *buf, size_t size)
{
    if (!dev || !buf || size == 0) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->state != XY_DEV_STATE_OPENED) {
        return XY_ERROR_NOT_READY;
    }
    if (!(dev->flags & XY_DEV_FLAG_RDWR) && !(dev->flags & XY_DEV_FLAG_RDONLY)) {
        return XY_ERROR_ACCESS_DENIED;
    }
    if (dev->api && dev->api->read) {
        return dev->api->read(dev, pos, buf, size);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

int32_t xy_device_write(xy_device_t *dev, uint32_t pos, const void *buf, size_t size)
{
    if (!dev || !buf || size == 0) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->state != XY_DEV_STATE_OPENED) {
        return XY_ERROR_NOT_READY;
    }
    if (!(dev->flags & XY_DEV_FLAG_RDWR) && !(dev->flags & XY_DEV_FLAG_WRONLY)) {
        return XY_ERROR_ACCESS_DENIED;
    }
    if (dev->api && dev->api->write) {
        return dev->api->write(dev, pos, buf, size);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_control(xy_device_t *dev, uint32_t cmd, void *args)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->api && dev->api->control) {
        return dev->api->control(dev, cmd, args);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_get_info(xy_device_t *dev, xy_dev_info_t *info)
{
    if (!dev || !info) {
        return XY_ERROR_INVALID_PARAM;
    }
    info->name = dev->name;
    info->type = dev->type;
    info->flags = dev->flags;
    info->state = dev->state;
    info->max_data_size = 0;
    info->buffer_size = 0;
    info->version = 0x020000;
    return XY_OK;
}

xy_dev_state_t xy_device_get_state(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEV_STATE_ERROR;
    }
    return dev->state;
}

/* ==================== Enumeration ==================== */

typedef struct {
    xy_dev_type_t filter;
    const char **names;
    uint32_t max_count;
    uint32_t count;
} enumerate_ctx_t;

static int enumerate_cb(xy_device_t *dev, void *arg)
{
    enumerate_ctx_t *ctx = (enumerate_ctx_t *)arg;
    if (ctx->count >= ctx->max_count) {
        return -1;
    }
    if (ctx->filter == XY_DEV_TYPE_MAX || dev->type == ctx->filter) {
        ctx->names[ctx->count++] = dev->name;
    }
    return 0;
}

uint32_t xy_device_enumerate(xy_dev_type_t type, const char **names, uint32_t max_count)
{
    if (!names || max_count == 0) {
        return 0;
    }
    enumerate_ctx_t ctx = { type, names, max_count, 0 };
    xy_device_foreach(enumerate_cb, &ctx);
    return ctx.count;
}

typedef struct {
    xy_dev_type_t filter;
    uint32_t count;
} count_ctx_t;

static int count_cb(xy_device_t *dev, void *arg)
{
    count_ctx_t *ctx = (count_ctx_t *)arg;
    if (ctx->filter == XY_DEV_TYPE_MAX || dev->type == ctx->filter) {
        ctx->count++;
    }
    return 0;
}

uint32_t xy_device_get_count(xy_dev_type_t type)
{
    count_ctx_t ctx = { type, 0 };
    xy_device_foreach(count_cb, &ctx);
    return ctx.count;
}

/* ==================== Power management (vtable dispatch) ==================== */

xy_error_t xy_device_set_power_mode(xy_device_t *dev, uint8_t power_mode)
{
    if (!dev) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->api && dev->api->power_control) {
        return dev->api->power_control(dev, power_mode);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_get_power_mode(xy_device_t *dev, uint8_t *power_mode)
{
    if (!dev || !power_mode) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->api && dev->api->control) {
        return dev->api->control(dev, XY_DEV_CMD_GET_POWER, power_mode);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

/* ==================== Async ==================== */

xy_error_t xy_device_async_read(xy_device_t *dev, uint32_t pos, void *buf,
                                size_t size, xy_async_callback_t cb, void *arg)
{
    if (!dev || !buf || size == 0) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->state != XY_DEV_STATE_OPENED) {
        return XY_ERROR_NOT_READY;
    }
    if (!(dev->flags & XY_DEV_FLAG_RDWR) && !(dev->flags & XY_DEV_FLAG_RDONLY)) {
        return XY_ERROR_ACCESS_DENIED;
    }
    if (dev->api && dev->api->async_read) {
        return dev->api->async_read(dev, pos, buf, size, cb, arg);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

xy_error_t xy_device_async_write(xy_device_t *dev, uint32_t pos, const void *buf,
                                 size_t size, xy_async_callback_t cb, void *arg)
{
    if (!dev || !buf || size == 0) {
        return XY_ERROR_INVALID_PARAM;
    }
    if (dev->state != XY_DEV_STATE_OPENED) {
        return XY_ERROR_NOT_READY;
    }
    if (!(dev->flags & XY_DEV_FLAG_RDWR) && !(dev->flags & XY_DEV_FLAG_WRONLY)) {
        return XY_ERROR_ACCESS_DENIED;
    }
    if (dev->api && dev->api->async_write) {
        return dev->api->async_write(dev, pos, buf, size, cb, arg);
    }
    return XY_ERROR_NOT_SUPPORTED;
}

/* ==================== Bus device adapters ==================== */

xy_error_t xy_bus_take(xy_bus_device_t *bus)
{
    if (!bus || !bus->bus_api || !bus->bus_api->take_bus) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->take_bus(&bus->parent);
}

xy_error_t xy_bus_release(xy_bus_device_t *bus)
{
    if (!bus || !bus->bus_api || !bus->bus_api->release_bus) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->release_bus(&bus->parent);
}

xy_error_t xy_bus_transfer(xy_bus_device_t *bus, xy_bus_node_t *node,
                          const void *send_buf, void *recv_buf, size_t length)
{
    if (!bus || !node || !bus->bus_api || !bus->bus_api->transfer) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->transfer(&bus->parent, &node->parent,
                                 send_buf, recv_buf, length);
}

xy_error_t xy_bus_configure(xy_bus_device_t *bus, xy_bus_node_t *node,
                            const void *config)
{
    if (!bus || !node || !bus->bus_api || !bus->bus_api->configure) {
        return XY_ERROR_INVALID_PARAM;
    }
    return bus->bus_api->configure(&bus->parent, &node->parent, config);
}

/* ==================== Sensor adapters ==================== */

xy_error_t xy_sensor_sample_fetch(void *sensor, xy_sensor_type_t channel)
{
    if (!sensor) {
        return XY_ERROR_INVALID_PARAM;
    }
    xy_sensor_device_t *sdev = (xy_sensor_device_t *)sensor;
    if (!sdev->sensor_api || !sdev->sensor_api->sample_fetch) {
        return XY_ERROR_NOT_SUPPORTED;
    }
    return sdev->sensor_api->sample_fetch(sensor, channel);
}

xy_error_t xy_sensor_channel_get(void *sensor, xy_sensor_type_t channel,
                                xy_sensor_value_t *val)
{
    if (!sensor || !val) {
        return XY_ERROR_INVALID_PARAM;
    }
    xy_sensor_device_t *sdev = (xy_sensor_device_t *)sensor;
    if (!sdev->sensor_api || !sdev->sensor_api->channel_get) {
        return XY_ERROR_NOT_SUPPORTED;
    }
    return sdev->sensor_api->channel_get(sensor, channel, val);
}

/* ==================== Convenience ==================== */

int xy_device_exists(const char *name)
{
    return xy_device_find(name) != NULL ? 1 : 0;
}

xy_dev_type_t xy_device_get_type(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev ? dev->type : XY_DEV_TYPE_MAX;
}

uint32_t xy_device_get_flags(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev ? dev->flags : 0;
}

int xy_device_is_opened(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev && (dev->state == XY_DEV_STATE_OPENED) ? 1 : 0;
}

int xy_device_get_ref_count(const char *name)
{
    xy_device_t *dev = xy_device_find(name);
    return dev ? dev->ref_count : 0;
}

/* ==================== Device manager (group wrapper) ==================== */

xy_error_t xy_device_mgr_init(void)
{
    return xy_device_init();
}

xy_error_t xy_device_mgr_register(const char *group, xy_device_t *dev)
{
    XY_UNUSED(group);
    return xy_device_register(dev);
}

xy_device_t *xy_device_mgr_find_by_group(const char *group, int index)
{
    XY_UNUSED(group);
    return xy_device_find_by_type(XY_DEV_TYPE_MAX, (size_t)index);
}

uint32_t xy_device_mgr_get_group_count(const char *group)
{
    XY_UNUSED(group);
    return (uint32_t)xy_device_registry_count();
}
