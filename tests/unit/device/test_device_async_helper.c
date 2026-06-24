/**
 * @file test_device_async_helper.c
 * @brief Unit tests for the optional xy_device_async_*_ex helper API.
 */

#include <string.h>

#include "unity.h"
#include "xy_device_async.h"
#include "xy_os.h"

static uint32_t g_fake_tick = 0;
static int g_callback_count = 0;
static int g_callback_result = 0;
static xy_device_async_op_t g_callback_op = XY_DEVICE_ASYNC_OP_NONE;
static int g_poll_result = 0;

#define TEST(name) static void name(void)
#define ASSERT(cond) TEST_ASSERT_TRUE(cond)

void setUp(void)
{
}

void tearDown(void)
{
}

uint32_t xy_os_tick_get(void)
{
    return g_fake_tick;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    g_fake_tick += ticks;
    return XY_OS_OK;
}

static void reset_fakes(void)
{
    g_fake_tick = 0;
    g_callback_count = 0;
    g_callback_result = 0;
    g_callback_op = XY_DEVICE_ASYNC_OP_NONE;
    g_poll_result = 0;
}

static void async_callback(xy_device_t *dev, xy_device_async_op_t op, int result, void *user_data)
{
    (void)dev;
    (void)user_data;
    g_callback_count++;
    g_callback_op = op;
    g_callback_result = result;
}

static int backend_read_pending(xy_device_t *dev, void *buffer, size_t length,
                                xy_device_async_callback_t callback, void *user_data,
                                uint32_t timeout_ms)
{
    (void)dev;
    (void)buffer;
    (void)length;
    (void)callback;
    (void)user_data;
    (void)timeout_ms;
    return XY_DEVICE_OK;
}

static int backend_poll_controlled(xy_device_t *dev)
{
    (void)dev;
    return g_poll_result;
}

static int backend_ready_false(xy_device_t *dev, bool for_write)
{
    (void)dev;
    (void)for_write;
    return 0;
}

TEST(test_init_rejects_null_context)
{
    ASSERT(xy_device_async_init_ex(NULL, NULL) == XY_DEVICE_INVALID_PARAM);
}

TEST(test_fallback_read_completes_immediately)
{
    xy_device_t dev = {0};
    xy_device_async_context_t ctx;
    xy_device_async_state_t state = XY_DEVICE_ASYNC_STATE_IDLE;
    size_t transferred = 0;
    uint8_t buffer[4] = {0};

    reset_fakes();
    ASSERT(xy_device_async_init_ex(&ctx, NULL) == XY_DEVICE_OK);
    ASSERT(xy_device_async_read_ex(&dev, &ctx, buffer, sizeof(buffer),
                                   async_callback, NULL, 0) == XY_DEVICE_OK);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_COMPLETED);
    ASSERT(xy_device_async_get_transferred_ex(&ctx, &transferred) == XY_DEVICE_OK);
    ASSERT(transferred == sizeof(buffer));
    ASSERT(g_callback_count == 1);
    ASSERT(g_callback_op == XY_DEVICE_ASYNC_OP_READ);
    ASSERT(g_callback_result == (int)sizeof(buffer));
}

TEST(test_backend_pending_busy_and_poll_completion)
{
    xy_device_t dev = {0};
    xy_device_async_context_t ctx;
    xy_device_async_state_t state = XY_DEVICE_ASYNC_STATE_IDLE;
    uint8_t buffer[8] = {0};
    const xy_device_async_ops_t ops = {
        .read = backend_read_pending,
        .poll = backend_poll_controlled,
        .ready = backend_ready_false,
    };

    reset_fakes();
    ASSERT(xy_device_async_init_ex(&ctx, &ops) == XY_DEVICE_OK);
    ASSERT(xy_device_async_read_ex(&dev, &ctx, buffer, sizeof(buffer),
                                   async_callback, NULL, 0) == XY_DEVICE_OK);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_PENDING);
    ASSERT(xy_device_async_ready_ex(&dev, &ctx, false) == 0);
    ASSERT(xy_device_async_write_ex(&dev, &ctx, buffer, sizeof(buffer),
                                    async_callback, NULL, 0) == XY_DEVICE_BUSY);

    ASSERT(xy_device_async_poll_ex(&dev, &ctx) == 0);
    g_poll_result = 3;
    ASSERT(xy_device_async_poll_ex(&dev, &ctx) == 1);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_COMPLETED);
    ASSERT(g_callback_count == 1);
    ASSERT(g_callback_result == 3);
}

TEST(test_wait_timeout_cancels_pending_request)
{
    xy_device_t dev = {0};
    xy_device_async_context_t ctx;
    xy_device_async_state_t state = XY_DEVICE_ASYNC_STATE_IDLE;
    uint8_t buffer[2] = {0};
    const xy_device_async_ops_t ops = {
        .read = backend_read_pending,
        .poll = backend_poll_controlled,
    };

    reset_fakes();
    ASSERT(xy_device_async_init_ex(&ctx, &ops) == XY_DEVICE_OK);
    ASSERT(xy_device_async_read_ex(&dev, &ctx, buffer, sizeof(buffer),
                                   async_callback, NULL, 0) == XY_DEVICE_OK);
    ASSERT(xy_device_async_wait_ex(&dev, &ctx, 2) == XY_DEVICE_TIMEOUT);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_ERROR);
    ASSERT(g_callback_count == 1);
    ASSERT(g_callback_result == XY_DEVICE_BUSY);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_rejects_null_context);
    RUN_TEST(test_fallback_read_completes_immediately);
    RUN_TEST(test_backend_pending_busy_and_poll_completion);
    RUN_TEST(test_wait_timeout_cancels_pending_request);

    return UNITY_END();
}
