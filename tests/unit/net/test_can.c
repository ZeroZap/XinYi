/**
 * @file test_can.c
 * @brief Unit tests for the rehabilitated CAN component API.
 */
#include "unity.h"
#include "fff.h"

#include "xy_can.h"

#include <string.h>

static uint32_t g_tick;
static xy_can_msg_t g_last_callback_msg;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VOID_FUNC(xy_os_delay, uint32_t)
FAKE_VOID_FUNC(on_can_rx, xy_can_t *, const xy_can_msg_t *)

static void on_can_rx_impl(xy_can_t *can, const xy_can_msg_t *msg);

static uint32_t xy_os_tick_get_impl(void)
{
    return g_tick;
}

static void xy_os_delay_impl(uint32_t ticks)
{
    g_tick += ticks;
}

void setUp(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_os_delay);
    RESET_FAKE(on_can_rx);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.custom_fake = xy_os_tick_get_impl;
    xy_os_delay_fake.custom_fake = xy_os_delay_impl;
    on_can_rx_fake.custom_fake = on_can_rx_impl;

    g_tick = 0;
    memset(&g_last_callback_msg, 0, sizeof(g_last_callback_msg));
}

void tearDown(void)
{
}

static void on_can_rx_impl(xy_can_t *can, const xy_can_msg_t *msg)
{
    TEST_ASSERT_NOT_NULL(can);
    TEST_ASSERT_NOT_NULL(msg);
    g_last_callback_msg = *msg;
}

static xy_can_msg_t make_msg(uint32_t id, uint8_t value)
{
    xy_can_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.id = id;
    msg.len = 2;
    msg.data[0] = value;
    msg.data[1] = (uint8_t)(value + 1U);
    return msg;
}

static void test_can_fifo_rx_callback(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 500000,
        .rx_fifo_size = 4,
        .tx_fifo_size = 4,
    };
    xy_can_msg_t tx = make_msg(0x123, 0x10);
    xy_can_msg_t rx;
    float rx_usage = -1.0F;
    float tx_usage = -1.0F;

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_TRUE(can.initialized);
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_start(&can));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));

    xy_can_isr_receive(&can, &tx);
    TEST_ASSERT_EQUAL_UINT(1U, on_can_rx_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&can, on_can_rx_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&tx, on_can_rx_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(tx.id, g_last_callback_msg.id);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));

    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(tx.id, rx.id);
    TEST_ASSERT_EQUAL_HEX8(tx.data[0], rx.data[0]);
    TEST_ASSERT_EQUAL_UINT(2U, on_can_rx_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&can, on_can_rx_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&rx, on_can_rx_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(2U, xy_can_get_rx_count(&can));

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_send(&can, &tx, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_tx_count(&can));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_get_fifo_usage(&can, &rx_usage, &tx_usage));
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0F, rx_usage);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0F, tx_usage);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_unregister_rx_callback(&can));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_stop(&can));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

static void test_can_timeout_and_direct_mode(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 250000,
        .rx_fifo_size = 0,
        .tx_fifo_size = 0,
    };
    xy_can_msg_t msg = make_msg(0x321, 0x20);
    xy_can_msg_t rx;

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_send(&can, &msg, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_tx_count(&can));

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));
    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT(1U, on_can_rx_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&can, on_can_rx_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&rx, on_can_rx_fake.arg1_val);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));

    config.rx_fifo_size = 2;
    config.tx_fifo_size = 1;
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_TIMEOUT, xy_can_send(&can, &msg, 2));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_delay_fake.arg0_val);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2U, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

static void test_can_timeout_preserves_counters_and_rx_output(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 500000,
        .rx_fifo_size = 2,
        .tx_fifo_size = 1,
    };
    xy_can_msg_t msg = make_msg(0x456, 0x30);
    xy_can_msg_t rx = make_msg(0x7FF, 0xA0);
    xy_can_msg_t rx_before = rx;

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));

    TEST_ASSERT_EQUAL(XY_CAN_TIMEOUT, xy_can_send(&can, &msg, 2));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_can_get_tx_count(&can));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_error_count(&can));

    TEST_ASSERT_EQUAL(XY_CAN_TIMEOUT, xy_can_receive(&can, &rx, 2));
    TEST_ASSERT_EQUAL_MEMORY(&rx_before, &rx, sizeof(rx));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT32(2U, xy_can_get_error_count(&can));
    TEST_ASSERT_EQUAL_UINT(0U, on_can_rx_fake.call_count);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

static void test_can_fifo_overflow_counts_error_without_rx_count(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 500000,
        .rx_fifo_size = 2,
        .tx_fifo_size = 0,
    };
    xy_can_msg_t first = make_msg(0x100, 0x11);
    xy_can_msg_t overflow = make_msg(0x101, 0x22);
    xy_can_msg_t rx;

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));

    xy_can_isr_receive(&can, &first);
    xy_can_isr_receive(&can, &overflow);

    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_error_count(&can));
    TEST_ASSERT_EQUAL_UINT(1U, on_can_rx_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(first.id, g_last_callback_msg.id);

    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(first.id, rx.id);
    TEST_ASSERT_EQUAL_HEX8(first.data[0], rx.data[0]);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

static void test_can_rejects_oversized_frames_without_side_effects(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 500000,
        .rx_fifo_size = 2,
        .tx_fifo_size = 1,
    };
    xy_can_msg_t oversized = make_msg(0x555, 0x44);
    xy_can_msg_t rx;

    oversized.len = (uint8_t)(sizeof(oversized.data) + 1U);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_INVALID_PARAM, xy_can_send(&can, &oversized, 0));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_can_get_tx_count(&can));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_can_get_error_count(&can));

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));
    xy_can_isr_receive(&can, &oversized);
    TEST_ASSERT_EQUAL_UINT32(0U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_error_count(&can));
    TEST_ASSERT_EQUAL_UINT(0U, on_can_rx_fake.call_count);

    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_TIMEOUT, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(0U, rx.id);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

static void test_can_callback_registration_requires_initialized_callback(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 500000,
        .rx_fifo_size = 2,
        .tx_fifo_size = 1,
    };

    memset(&can, 0, sizeof(can));
    TEST_ASSERT_EQUAL(XY_CAN_INVALID_PARAM,
                      xy_can_register_rx_callback(NULL, on_can_rx, (void *)0x1234));
    TEST_ASSERT_EQUAL(XY_CAN_INVALID_PARAM,
                      xy_can_register_rx_callback(&can, on_can_rx, (void *)0x1234));
    TEST_ASSERT_NULL(can.rx_callback);
    TEST_ASSERT_NULL(can.callback_user_data);
    TEST_ASSERT_EQUAL(XY_CAN_INVALID_PARAM, xy_can_unregister_rx_callback(&can));

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_INVALID_PARAM,
                      xy_can_register_rx_callback(&can, NULL, (void *)0x1234));
    TEST_ASSERT_NULL(can.rx_callback);
    TEST_ASSERT_NULL(can.callback_user_data);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, (void *)0x1234));
    TEST_ASSERT_EQUAL_PTR(on_can_rx, can.rx_callback);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, can.callback_user_data);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
    TEST_ASSERT_EQUAL(XY_CAN_INVALID_PARAM, xy_can_unregister_rx_callback(&can));
    TEST_ASSERT_EQUAL_PTR(on_can_rx, can.rx_callback);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, can.callback_user_data);
}

static void test_can_unregister_suppresses_direct_mode_callback(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 125000,
        .rx_fifo_size = 0,
        .tx_fifo_size = 0,
    };
    xy_can_msg_t rx = make_msg(0x200, 0x55);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_unregister_rx_callback(&can));
    TEST_ASSERT_NULL(can.rx_callback);
    TEST_ASSERT_NULL(can.callback_user_data);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT(0U, on_can_rx_fake.call_count);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

static void test_can_unregister_suppresses_fifo_callbacks(void)
{
    xy_can_t can;
    xy_can_config_t config = {
        .baudrate = 500000,
        .rx_fifo_size = 3,
        .tx_fifo_size = 0,
    };
    xy_can_msg_t queued = make_msg(0x201, 0x66);
    xy_can_msg_t rx;

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_unregister_rx_callback(&can));

    xy_can_isr_receive(&can, &queued);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT(0U, on_can_rx_fake.call_count);

    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(queued.id, rx.id);
    TEST_ASSERT_EQUAL_HEX8(queued.data[0], rx.data[0]);
    TEST_ASSERT_EQUAL_UINT32(2U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT(0U, on_can_rx_fake.call_count);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_can_fifo_rx_callback);
    RUN_TEST(test_can_timeout_and_direct_mode);
    RUN_TEST(test_can_timeout_preserves_counters_and_rx_output);
    RUN_TEST(test_can_fifo_overflow_counts_error_without_rx_count);
    RUN_TEST(test_can_rejects_oversized_frames_without_side_effects);
    RUN_TEST(test_can_callback_registration_requires_initialized_callback);
    RUN_TEST(test_can_unregister_suppresses_direct_mode_callback);
    RUN_TEST(test_can_unregister_suppresses_fifo_callbacks);
    return UNITY_END();
}
