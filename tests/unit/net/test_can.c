/**
 * @file test_can.c
 * @brief Unit tests for the rehabilitated CAN component API.
 */
#include "unity.h"

#include "xy_can.h"

#include <string.h>

static uint32_t g_tick;
static uint32_t g_delay_calls;
static uint32_t g_callback_count;
static xy_can_msg_t g_last_callback_msg;

uint32_t xy_os_tick_get(void)
{
    return g_tick;
}

void xy_os_delay(uint32_t ticks)
{
    g_delay_calls++;
    g_tick += ticks;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void on_can_rx(xy_can_t *can, const xy_can_msg_t *msg)
{
    TEST_ASSERT_NOT_NULL(can);
    TEST_ASSERT_NOT_NULL(msg);
    g_callback_count++;
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

    g_tick = 0;
    g_delay_calls = 0;
    g_callback_count = 0;
    memset(&g_last_callback_msg, 0, sizeof(g_last_callback_msg));

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_TRUE(can.initialized);
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_start(&can));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));

    xy_can_isr_receive(&can, &tx);
    TEST_ASSERT_EQUAL_UINT32(1U, g_callback_count);
    TEST_ASSERT_EQUAL_UINT32(tx.id, g_last_callback_msg.id);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));

    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(tx.id, rx.id);
    TEST_ASSERT_EQUAL_HEX8(tx.data[0], rx.data[0]);
    TEST_ASSERT_EQUAL_UINT32(2U, g_callback_count);
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

    g_tick = 0;
    g_delay_calls = 0;
    g_callback_count = 0;

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_send(&can, &msg, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_tx_count(&can));

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_register_rx_callback(&can, on_can_rx, NULL));
    memset(&rx, 0, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_receive(&can, &rx, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_can_get_rx_count(&can));
    TEST_ASSERT_EQUAL_UINT32(1U, g_callback_count);

    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));

    config.rx_fifo_size = 2;
    config.tx_fifo_size = 1;
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_init(&can, NULL, &config));
    TEST_ASSERT_EQUAL(XY_CAN_TIMEOUT, xy_can_send(&can, &msg, 2));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2U, g_delay_calls);
    TEST_ASSERT_EQUAL(XY_CAN_OK, xy_can_deinit(&can));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_can_fifo_rx_callback);
    RUN_TEST(test_can_timeout_and_direct_mode);
    return UNITY_END();
}
