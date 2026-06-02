/**
 * @file xy_device_async.c
 * @brief Optional queued async helper implementation.
 * @version 1.0.0
 * @date 2026-03-15
 *
 * @note Generic xy_device_async_read/write dispatch lives in xy_device.c.
 *       This file implements the distinct xy_device_async_*_ex helper API
 *       declared by xy_device_async.h.
 */

#include "inc/xy_device_async.h"
#include "xy_os.h"
#include <string.h>

/* ==================== Private Functions ==================== */

static uint32_t async_get_tick_ms(void)
{
    return xy_os_tick_get();
}

static bool async_check_timeout(const xy_device_async_request_t *req)
{
    if (!req || req->timeout_ms == 0) {
        return false;
    }

    uint32_t now = async_get_tick_ms();
    uint32_t elapsed = now - req->start_time;

    return elapsed >= req->timeout_ms;
}

static void async_complete_request(xy_device_t *dev, xy_device_async_context_t *ctx, int result)
{
    if (!ctx || !ctx->initialized) {
        return;
    }

    xy_device_async_request_t *req = &ctx->request;

    if (result >= 0) {
        req->state = XY_DEVICE_ASYNC_STATE_COMPLETED;
        req->transferred = (size_t)result;
        req->error_code = 0;
    } else {
        req->state = XY_DEVICE_ASYNC_STATE_ERROR;
        req->error_code = result;
    }

    ctx->is_busy = false;

    if (req->callback) {
        req->callback(dev, req->op, result, req->user_data);
    }
}

/* ==================== Public Implementation ==================== */

int xy_device_async_init_ex(xy_device_async_context_t *ctx,
                            const xy_device_async_ops_t *ops)
{
    if (!ctx) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->ops = ops;
    ctx->initialized = true;
    ctx->request.state = XY_DEVICE_ASYNC_STATE_IDLE;

    return XY_DEVICE_OK;
}

int xy_device_async_read_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                            void *buffer, size_t length,
                            xy_device_async_callback_t callback, void *user_data,
                            uint32_t timeout_ms)
{
    if (!dev || !ctx || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    if (ctx->is_busy) {
        return XY_DEVICE_BUSY;
    }

    xy_device_async_request_t *req = &ctx->request;
    req->op = XY_DEVICE_ASYNC_OP_READ;
    req->state = XY_DEVICE_ASYNC_STATE_PENDING;
    req->buffer = buffer;
    req->length = length;
    req->transferred = 0;
    req->error_code = 0;
    req->callback = callback;
    req->user_data = user_data;
    req->timeout_ms = timeout_ms;
    req->start_time = async_get_tick_ms();

    ctx->is_busy = true;

    if (ctx->ops && ctx->ops->read) {
        int ret = ctx->ops->read(dev, buffer, length, callback, user_data, timeout_ms);
        if (ret != XY_DEVICE_OK) {
            ctx->is_busy = false;
            req->state = XY_DEVICE_ASYNC_STATE_ERROR;
            req->error_code = ret;
            return ret;
        }
        return XY_DEVICE_OK;
    }

    async_complete_request(dev, ctx, (int)length);
    return XY_DEVICE_OK;
}

int xy_device_async_write_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                             const void *buffer, size_t length,
                             xy_device_async_callback_t callback, void *user_data,
                             uint32_t timeout_ms)
{
    if (!dev || !ctx || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    if (ctx->is_busy) {
        return XY_DEVICE_BUSY;
    }

    xy_device_async_request_t *req = &ctx->request;
    req->op = XY_DEVICE_ASYNC_OP_WRITE;
    req->state = XY_DEVICE_ASYNC_STATE_PENDING;
    req->buffer = (void *)buffer;
    req->length = length;
    req->transferred = 0;
    req->error_code = 0;
    req->callback = callback;
    req->user_data = user_data;
    req->timeout_ms = timeout_ms;
    req->start_time = async_get_tick_ms();

    ctx->is_busy = true;

    if (ctx->ops && ctx->ops->write) {
        int ret = ctx->ops->write(dev, buffer, length, callback, user_data, timeout_ms);
        if (ret != XY_DEVICE_OK) {
            ctx->is_busy = false;
            req->state = XY_DEVICE_ASYNC_STATE_ERROR;
            req->error_code = ret;
            return ret;
        }
        return XY_DEVICE_OK;
    }

    async_complete_request(dev, ctx, (int)length);
    return XY_DEVICE_OK;
}

int xy_device_async_cancel_ex(xy_device_t *dev, xy_device_async_context_t *ctx)
{
    if (!ctx) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    if (!ctx->is_busy) {
        return XY_DEVICE_OK;
    }

    ctx->is_busy = false;
    ctx->request.state = XY_DEVICE_ASYNC_STATE_ERROR;
    ctx->request.error_code = XY_DEVICE_BUSY;

    if (ctx->request.callback) {
        ctx->request.callback(dev, ctx->request.op, XY_DEVICE_BUSY,
                              ctx->request.user_data);
    }

    return XY_DEVICE_OK;
}

int xy_device_async_get_state_ex(const xy_device_async_context_t *ctx,
                                 xy_device_async_state_t *state)
{
    if (!ctx || !state) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }

    *state = ctx->request.state;
    return XY_DEVICE_OK;
}

int xy_device_async_get_transferred_ex(const xy_device_async_context_t *ctx,
                                       size_t *transferred)
{
    if (!ctx || !transferred) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }

    *transferred = ctx->request.transferred;
    return XY_DEVICE_OK;
}

int xy_device_async_poll_ex(xy_device_t *dev, xy_device_async_context_t *ctx)
{
    if (!dev || !ctx) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    if (!ctx->is_busy) {
        return 0;
    }

    if (async_check_timeout(&ctx->request)) {
        async_complete_request(dev, ctx, XY_DEVICE_TIMEOUT);
        return 1;
    }

    if (ctx->ops && ctx->ops->poll) {
        int ret = ctx->ops->poll(dev);
        if (ret != 0) {
            async_complete_request(dev, ctx, ret > 0 ? ret : XY_DEVICE_OK);
            return 1;
        }
        return 0;
    }

    async_complete_request(dev, ctx, (int)ctx->request.length);
    return 1;
}

int xy_device_async_wait_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                            uint32_t timeout_ms)
{
    if (!dev || !ctx) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return XY_DEVICE_NOT_INIT;
    }
    if (!ctx->is_busy) {
        return XY_DEVICE_OK;
    }

    uint32_t start = async_get_tick_ms();

    while (ctx->is_busy) {
        int ret = xy_device_async_poll_ex(dev, ctx);
        if (ret != 0) {
            return ctx->request.state == XY_DEVICE_ASYNC_STATE_COMPLETED ?
                   XY_DEVICE_OK : XY_DEVICE_TIMEOUT;
        }

        if (timeout_ms > 0 && (async_get_tick_ms() - start) >= timeout_ms) {
            xy_device_async_cancel_ex(dev, ctx);
            return XY_DEVICE_TIMEOUT;
        }

        xy_os_delay(1);
    }

    return XY_DEVICE_OK;
}

int xy_device_async_ready_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                             bool for_write)
{
    if (!dev || !ctx) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if (ctx->ops && ctx->ops->ready) {
        return ctx->ops->ready(dev, for_write);
    }

    return ctx->is_busy ? 0 : 1;
}

/* ==================== End of File ==================== */
