/**
 * @file test_device_async_helper.c
 * @brief Unit tests for the optional xy_device_async_*_ex helper API.
 */

#include <string.h>

#include "unity.h"
#include "fff.h"
#include "xy_device_async.h"
#include "xy_os.h"

#define TEST(name) static void name(void)
#define ASSERT(cond) TEST_ASSERT_TRUE(cond)

static uint32_t g_fake_tick = 0;
static int g_poll_result = 0;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VALUE_FUNC(xy_os_status_t, xy_os_delay, uint32_t)
FAKE_VOID_FUNC(async_callback, xy_device_t *, xy_device_async_op_t, int, void *)
FAKE_VALUE_FUNC(int, backend_read_pending, xy_device_t *, void *, size_t,
                xy_device_async_callback_t, void *, uint32_t)
FAKE_VALUE_FUNC(int, backend_poll_controlled, xy_device_t *)
FAKE_VALUE_FUNC(int, backend_ready_false, xy_device_t *, bool)

static uint32_t xy_os_tick_get_impl(void)
{
    return g_fake_tick;
}

static xy_os_status_t xy_os_delay_impl(uint32_t ticks)
{
    g_fake_tick += ticks;
    return XY_OS_OK;
}

static int backend_poll_controlled_impl(xy_device_t *dev)
{
    (void)dev;
    return g_poll_result;
}

static void reset_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_os_delay);
    RESET_FAKE(async_callback);
    RESET_FAKE(backend_read_pending);
    RESET_FAKE(backend_poll_controlled);
    RESET_FAKE(backend_ready_false);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.custom_fake = xy_os_tick_get_impl;
    xy_os_delay_fake.custom_fake = xy_os_delay_impl;
    backend_read_pending_fake.return_val = XY_DEVICE_OK;
    backend_poll_controlled_fake.custom_fake = backend_poll_controlled_impl;
    backend_ready_false_fake.return_val = 0;

    g_fake_tick = 0;
    g_poll_result = 0;
}

void setUp(void)
{
    reset_fakes();
}

void tearDown(void)
{
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

    ASSERT(xy_device_async_init_ex(&ctx, NULL) == XY_DEVICE_OK);
    ASSERT(xy_device_async_read_ex(&dev, &ctx, buffer, sizeof(buffer),
                                   async_callback, NULL, 0) == XY_DEVICE_OK);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_COMPLETED);
    ASSERT(xy_device_async_get_transferred_ex(&ctx, &transferred) == XY_DEVICE_OK);
    ASSERT(transferred == sizeof(buffer));
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, async_callback_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, async_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL(XY_DEVICE_ASYNC_OP_READ, async_callback_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT((int)sizeof(buffer), async_callback_fake.arg2_val);
    TEST_ASSERT_NULL(async_callback_fake.arg3_val);
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

    ASSERT(xy_device_async_init_ex(&ctx, &ops) == XY_DEVICE_OK);
    ASSERT(xy_device_async_read_ex(&dev, &ctx, buffer, sizeof(buffer),
                                   async_callback, NULL, 0) == XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_UINT(1U, backend_read_pending_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, backend_read_pending_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(buffer, backend_read_pending_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(buffer), backend_read_pending_fake.arg2_val);
    TEST_ASSERT_EQUAL_PTR(async_callback, backend_read_pending_fake.arg3_val);
    TEST_ASSERT_NULL(backend_read_pending_fake.arg4_val);
    TEST_ASSERT_EQUAL_UINT32(0U, backend_read_pending_fake.arg5_val);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_PENDING);
    ASSERT(xy_device_async_ready_ex(&dev, &ctx, false) == 0);
    TEST_ASSERT_EQUAL_UINT(1U, backend_ready_false_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, backend_ready_false_fake.arg0_val);
    TEST_ASSERT_FALSE(backend_ready_false_fake.arg1_val);
    ASSERT(xy_device_async_write_ex(&dev, &ctx, buffer, sizeof(buffer),
                                    async_callback, NULL, 0) == XY_DEVICE_BUSY);

    ASSERT(xy_device_async_poll_ex(&dev, &ctx) == 0);
    TEST_ASSERT_EQUAL_UINT(1U, backend_poll_controlled_fake.call_count);
    g_poll_result = 3;
    ASSERT(xy_device_async_poll_ex(&dev, &ctx) == 1);
    TEST_ASSERT_EQUAL_UINT(2U, backend_poll_controlled_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, backend_poll_controlled_fake.arg0_val);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_COMPLETED);
    TEST_ASSERT_EQUAL_UINT(1U, async_callback_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, async_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL(XY_DEVICE_ASYNC_OP_READ, async_callback_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT(3, async_callback_fake.arg2_val);
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

    ASSERT(xy_device_async_init_ex(&ctx, &ops) == XY_DEVICE_OK);
    ASSERT(xy_device_async_read_ex(&dev, &ctx, buffer, sizeof(buffer),
                                   async_callback, NULL, 0) == XY_DEVICE_OK);
    ASSERT(xy_device_async_wait_ex(&dev, &ctx, 2) == XY_DEVICE_TIMEOUT);
    ASSERT(xy_device_async_get_state_ex(&ctx, &state) == XY_DEVICE_OK);
    ASSERT(state == XY_DEVICE_ASYNC_STATE_ERROR);
    TEST_ASSERT_EQUAL_UINT(1U, backend_read_pending_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(3U, backend_poll_controlled_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_delay_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(1U, async_callback_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, async_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL(XY_DEVICE_ASYNC_OP_READ, async_callback_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, async_callback_fake.arg2_val);
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
