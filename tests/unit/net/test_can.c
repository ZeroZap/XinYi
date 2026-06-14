/**
 * @file test_can.c
 * @brief Unit tests for the rehabilitated CAN component API.
 */

#include "xy_can.h"

#include <assert.h>
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

static void on_can_rx(xy_can_t *can, const xy_can_msg_t *msg)
{
    assert(can != NULL);
    assert(msg != NULL);
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

    assert(xy_can_init(&can, NULL, &config) == XY_CAN_OK);
    assert(can.initialized);
    assert(xy_can_start(&can) == XY_CAN_OK);
    assert(xy_can_register_rx_callback(&can, on_can_rx, NULL) == XY_CAN_OK);

    xy_can_isr_receive(&can, &tx);
    assert(g_callback_count == 1U);
    assert(g_last_callback_msg.id == tx.id);
    assert(xy_can_get_rx_count(&can) == 1U);

    memset(&rx, 0, sizeof(rx));
    assert(xy_can_receive(&can, &rx, 0) == XY_CAN_OK);
    assert(rx.id == tx.id);
    assert(rx.data[0] == tx.data[0]);
    assert(g_callback_count == 2U);
    assert(xy_can_get_rx_count(&can) == 2U);

    assert(xy_can_send(&can, &tx, 0) == XY_CAN_OK);
    assert(xy_can_get_tx_count(&can) == 1U);
    assert(xy_can_get_fifo_usage(&can, &rx_usage, &tx_usage) == XY_CAN_OK);
    assert(rx_usage >= 0.0F);
    assert(tx_usage > 0.0F);

    assert(xy_can_unregister_rx_callback(&can) == XY_CAN_OK);
    assert(xy_can_stop(&can) == XY_CAN_OK);
    assert(xy_can_deinit(&can) == XY_CAN_OK);
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

    assert(xy_can_init(&can, NULL, &config) == XY_CAN_OK);
    assert(xy_can_send(&can, &msg, 0) == XY_CAN_OK);
    assert(xy_can_get_tx_count(&can) == 1U);

    assert(xy_can_register_rx_callback(&can, on_can_rx, NULL) == XY_CAN_OK);
    memset(&rx, 0, sizeof(rx));
    assert(xy_can_receive(&can, &rx, 0) == XY_CAN_OK);
    assert(xy_can_get_rx_count(&can) == 1U);
    assert(g_callback_count == 1U);

    assert(xy_can_deinit(&can) == XY_CAN_OK);

    config.rx_fifo_size = 2;
    config.tx_fifo_size = 1; /* one-slot ring buffer cannot accept data */
    assert(xy_can_init(&can, NULL, &config) == XY_CAN_OK);
    assert(xy_can_send(&can, &msg, 2) == XY_CAN_TIMEOUT);
    assert(g_delay_calls >= 2U);
    assert(xy_can_deinit(&can) == XY_CAN_OK);
}

int main(void)
{
    test_can_fifo_rx_callback();
    test_can_timeout_and_direct_mode();
    return 0;
}
