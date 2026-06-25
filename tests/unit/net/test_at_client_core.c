#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "at_client.h"

static const char *g_rx_data;
static size_t g_rx_len;
static size_t g_rx_pos;
static uint8_t g_tx_data[512];
static uint32_t g_tx_len;
static uint32_t g_tick;

static void mock_feed(const char *data)
{
    g_rx_data = data;
    g_rx_len = strlen(data);
    g_rx_pos = 0;
}

static int mock_read_byte(at_device_t *dev)
{
    (void)dev;
    if (g_rx_pos < g_rx_len) {
        return (uint8_t)g_rx_data[g_rx_pos++];
    }
    return -1;
}

static void mock_write(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    (void)dev;
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_tx_data), g_tx_len + len);
    memcpy(&g_tx_data[g_tx_len], data, len);
    g_tx_len += len;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static at_tick_t mock_get_tick(void)
{
    return g_tick++;
}

static void reset_io(void)
{
    g_rx_data = "";
    g_rx_len = 0;
    g_rx_pos = 0;
    g_tx_len = 0;
    g_tick = 0;
    memset(g_tx_data, 0, sizeof(g_tx_data));
}

static bool custom_resp_handler(at_device_t *dev, const char *resp_line,
                                void *user_data, at_resp_type_t *type)
{
    (void)dev;
    int *seen = (int *)user_data;
    if (strstr(resp_line, "+READY")) {
        (*seen)++;
        *type = AT_RESP_CUSTOM;
        return true;
    }
    return false;
}

static void test_device_registration_and_default_command(void)
{
    reset_io();
    at_client_t *client = at_client_init();
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_EQUAL_UINT8(0, client->device_count);

    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_UINT8(0, dev->id);
    TEST_ASSERT_EQUAL_STRING("modem", dev->name);
    TEST_ASSERT_EQUAL_UINT8(1, client->device_count);
    TEST_ASSERT_EQUAL_UINT8(0, client->default_device);

    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_cmd("AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT32(4, g_tx_len);
    TEST_ASSERT_EQUAL_MEMORY("AT\r\n", g_tx_data, 4);
    TEST_ASSERT_EQUAL_UINT32(4, dev->tx_count);
    TEST_ASSERT_EQUAL_UINT32(2, dev->rx_count);
}

static void test_send_command_validation_and_busy_state(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(NULL, "AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, NULL, NULL, NULL, 100));

    dev->is_busy = true;
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, "AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT32(0, g_tx_len);
    dev->is_busy = false;
}

static void test_ok_error_timeout_and_custom_response(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_command(dev, "AT", NULL, NULL, 100));

    reset_io();
    mock_feed("ERROR\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, "AT+BAD", NULL, NULL, 100));

    reset_io();
    mock_feed("");
    TEST_ASSERT_EQUAL(AT_RESP_TIMEOUT, at_send_command(dev, "AT", NULL, NULL, 3));

    reset_io();
    int seen = 0;
    mock_feed("+READY\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_CUSTOM, at_send_command(dev, "AT+READY", custom_resp_handler, &seen, 100));
    TEST_ASSERT_EQUAL_INT(1, seen);
}

static void test_raw_readline_expect_and_stats(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    const uint8_t raw[] = {0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_INT(-1, at_send_raw(NULL, raw, sizeof(raw)));
    TEST_ASSERT_EQUAL_INT(-1, at_send_raw(dev, NULL, sizeof(raw)));
    TEST_ASSERT_EQUAL_INT(-1, at_send_raw(dev, raw, 0));
    TEST_ASSERT_EQUAL_INT((int)sizeof(raw), at_send_raw(dev, raw, sizeof(raw)));

    char line[16];
    mock_feed("VALUE\r\n");
    TEST_ASSERT_EQUAL_INT(5, at_readline(dev, line, sizeof(line)));
    TEST_ASSERT_EQUAL_STRING("VALUE", line);

    reset_io();
    mock_feed("NOISE\r\nTARGET\r\n");
    TEST_ASSERT_TRUE(at_expect(dev, "TARGET", 100));

    uint32_t tx = 0, rx = 0, err = 0;
    at_get_stats(dev, &tx, &rx, &err);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(sizeof(raw), tx);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_device_registration_and_default_command);
    RUN_TEST(test_send_command_validation_and_busy_state);
    RUN_TEST(test_ok_error_timeout_and_custom_response);
    RUN_TEST(test_raw_readline_expect_and_stats);
    return UNITY_END();
}
