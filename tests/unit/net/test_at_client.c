/**
 * @file test_at_client.c
 * @brief AT Client complete smoke tests for the root tests CMake target
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define XY_PLATFORM_PC 1
#define XY_LOG_LEVEL 3

#include "at_client.h"
#include "unity.h"

static char rx_buf[256];
static size_t rx_len;
static size_t rx_pos;
static int tx_count;
static at_device_t *g_dev;

static uint32_t mock_get_tick(void)
{
    static uint32_t tick = 0;
    return tick++;
}

static void mock_feed(const char *data)
{
    rx_len = strlen(data);
    rx_pos = 0;
    if (rx_len < sizeof(rx_buf)) {
        memcpy(rx_buf, data, rx_len);
    }
}

static int mock_read_byte(at_device_t *dev)
{
    (void)dev;
    if (rx_pos < rx_len) {
        return (unsigned char)rx_buf[rx_pos++];
    }
    return -1;
}

static void mock_write(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    (void)dev;
    tx_count++;
    printf("[TX #%d] %.*s\n", tx_count, (int)len, (const char *)data);
}

void setUp(void)
{
    at_client_t *client = at_client_init();
    g_dev = at_device_register("test", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_NOT_NULL(g_dev);
    tx_count = 0;
    rx_len = 0;
    rx_pos = 0;
    g_dev->is_busy = false;
}

void tearDown(void)
{
}

static void test_null_command_is_rejected_without_tx(void)
{
    at_resp_type_t resp = at_send_command(g_dev, NULL, NULL, NULL, 1000);
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, resp);
    TEST_ASSERT_EQUAL_INT(0, tx_count);
}

static void test_busy_device_rejects_command_without_tx(void)
{
    g_dev->is_busy = true;
    at_resp_type_t resp = at_send_command(g_dev, "AT", NULL, NULL, 1000);
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, resp);
    TEST_ASSERT_EQUAL_INT(0, tx_count);
}

static void test_ok_response_is_detected(void)
{
    mock_feed("OK\r\n");
    at_resp_type_t resp = at_send_command(g_dev, "AT", NULL, NULL, 1000);
    TEST_ASSERT_EQUAL(AT_RESP_OK, resp);
    TEST_ASSERT_GREATER_THAN_INT(0, tx_count);
}

static void test_error_response_is_detected(void)
{
    mock_feed("ERROR\r\n");
    at_resp_type_t resp = at_send_command(g_dev, "AT+INVALID", NULL, NULL, 1000);
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, resp);
}

static void test_command_with_args_returns_ok(void)
{
    mock_feed("OK\r\n");
    at_resp_type_t resp = at_send_command(g_dev, "AT+BAUD=115200", NULL, NULL, 1000);
    TEST_ASSERT_EQUAL(AT_RESP_OK, resp);
}

static void test_statistics_update_after_command(void)
{
    uint32_t tx_before = g_dev->tx_count;
    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_command(g_dev, "AT", NULL, NULL, 1000));
    TEST_ASSERT_GREATER_THAN_UINT32(tx_before, g_dev->tx_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_command_is_rejected_without_tx);
    RUN_TEST(test_busy_device_rejects_command_without_tx);
    RUN_TEST(test_ok_response_is_detected);
    RUN_TEST(test_error_response_is_detected);
    RUN_TEST(test_command_with_args_returns_ok);
    RUN_TEST(test_statistics_update_after_command);
    return UNITY_END();
}
