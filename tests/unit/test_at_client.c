#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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
    assert(g_tx_len + len <= sizeof(g_tx_data));
    memcpy(&g_tx_data[g_tx_len], data, len);
    g_tx_len += len;
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
    assert(client != NULL);
    assert(client->device_count == 0);

    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    assert(dev != NULL);
    assert(dev->id == 0);
    assert(strcmp(dev->name, "modem") == 0);
    assert(client->device_count == 1);
    assert(client->default_device == 0);

    mock_feed("OK\r\n");
    assert(at_send_cmd("AT", NULL, NULL, 100) == AT_RESP_OK);
    assert(g_tx_len == 4);
    assert(memcmp(g_tx_data, "AT\r\n", 4) == 0);
    assert(dev->tx_count == 4);
    assert(dev->rx_count == 2);
}

static void test_send_command_validation_and_busy_state(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    assert(dev != NULL);

    assert(at_send_command(NULL, "AT", NULL, NULL, 100) == AT_RESP_ERROR);
    assert(at_send_command(dev, NULL, NULL, NULL, 100) == AT_RESP_ERROR);

    dev->is_busy = true;
    assert(at_send_command(dev, "AT", NULL, NULL, 100) == AT_RESP_ERROR);
    assert(g_tx_len == 0);
    dev->is_busy = false;
}

static void test_ok_error_timeout_and_custom_response(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    assert(dev != NULL);

    mock_feed("OK\r\n");
    assert(at_send_command(dev, "AT", NULL, NULL, 100) == AT_RESP_OK);

    reset_io();
    mock_feed("ERROR\r\n");
    assert(at_send_command(dev, "AT+BAD", NULL, NULL, 100) == AT_RESP_ERROR);

    reset_io();
    mock_feed("");
    assert(at_send_command(dev, "AT", NULL, NULL, 3) == AT_RESP_TIMEOUT);

    reset_io();
    int seen = 0;
    mock_feed("+READY\r\n");
    assert(at_send_command(dev, "AT+READY", custom_resp_handler, &seen, 100) == AT_RESP_CUSTOM);
    assert(seen == 1);
}

static void test_raw_readline_expect_and_stats(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    assert(dev != NULL);

    const uint8_t raw[] = {0x01, 0x02, 0x03};
    assert(at_send_raw(NULL, raw, sizeof(raw)) == -1);
    assert(at_send_raw(dev, NULL, sizeof(raw)) == -1);
    assert(at_send_raw(dev, raw, 0) == -1);
    assert(at_send_raw(dev, raw, sizeof(raw)) == (int)sizeof(raw));

    char line[16];
    mock_feed("VALUE\r\n");
    assert(at_readline(dev, line, sizeof(line)) == 5);
    assert(strcmp(line, "VALUE") == 0);

    reset_io();
    mock_feed("NOISE\r\nTARGET\r\n");
    assert(at_expect(dev, "TARGET", 100));

    uint32_t tx = 0, rx = 0, err = 0;
    at_get_stats(dev, &tx, &rx, &err);
    assert(tx >= sizeof(raw));
}

int main(void)
{
    test_device_registration_and_default_command();
    test_send_command_validation_and_busy_state();
    test_ok_error_timeout_and_custom_response();
    test_raw_readline_expect_and_stats();
    return 0;
}
