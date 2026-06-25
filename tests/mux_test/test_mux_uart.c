/**
 * @file test_mux_uart.c
 * @brief Focused host tests for MUX UART helper contracts
 */

#include "xy_mux.h"
#include "xy_mux_uart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unity.h"

#define BUFFER_SIZE 512U

static uint8_t g_last_write[256];
static size_t g_last_write_len;
static uint8_t g_read_pattern = 0x5A;
static int g_ioctl_count;

void xy_log_char(char ch)
{
    (void)ch;
}

static int32_t mock_uart_init(uint8_t channel, const void *config)
{
    (void)config;
    g_last_write_len = 0;
    g_ioctl_count = 0;
    printf("    [MOCK] UART-%u init\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_uart_deinit(uint8_t channel)
{
    printf("    [MOCK] UART-%u deinit\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_uart_write(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0 || len > sizeof(g_last_write)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memcpy(g_last_write, data, len);
    g_last_write_len = len;
    return (int32_t)len;
}

static int32_t mock_uart_read(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memset(data, g_read_pattern, len);
    return (int32_t)len;
}

static int32_t mock_uart_ioctl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    g_ioctl_count++;
    if (cmd == XY_MUX_UART_CMD_SET_CONFIG) {
        TEST_ASSERT_NOT_NULL(arg);
    }
    return XY_MUX_OK;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static xy_mux_manager_t make_mgr(uint8_t *tx, uint8_t *rx)
{
    xy_mux_manager_t mgr;
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_init(&mgr, tx, rx, BUFFER_SIZE));
    return mgr;
}

static xy_mux_ops_t make_ops(void)
{
    xy_mux_ops_t ops = {
        .init = mock_uart_init,
        .deinit = mock_uart_deinit,
        .read = mock_uart_read,
        .write = mock_uart_write,
        .ioctl = mock_uart_ioctl,
    };
    return ops;
}

static void test_uart_register_and_config(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 0, &ops, NULL));
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 1, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT8(2, mgr.device_count);

    xy_mux_uart_config_t cfg = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
        .flow_control = 0,
    };
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_config(&mgr, 0, &cfg));
    TEST_ASSERT_GREATER_THAN_INT(0, g_ioctl_count);

    xy_mux_deinit(&mgr);
}

static void test_uart_write_header_and_payload(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 0, &ops, NULL));

    const uint8_t data[] = {'h', 'e', 'l', 'l', 'o'};
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data), xy_mux_uart_write(&mgr, 0, data, sizeof(data), 250));
    TEST_ASSERT_EQUAL_size_t(sizeof(data), g_last_write_len);
    TEST_ASSERT_EQUAL_MEMORY(data, g_last_write, sizeof(data));

    xy_mux_deinit(&mgr);
}

static void test_uart_read_request_and_data(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 2, &ops, NULL));

    uint8_t data[6] = {0};
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data), xy_mux_uart_read(&mgr, 2, data, sizeof(data), 1000));
    for (size_t i = 0; i < sizeof(data); ++i) {
        TEST_ASSERT_EQUAL_HEX8(g_read_pattern, data[i]);
    }

    xy_mux_deinit(&mgr);
}

static void test_uart_error_paths(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    uint8_t data = 0x55;

    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_uart_write(NULL, 0, &data, 1, 10));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_uart_read(NULL, 0, &data, 1, 10));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_uart_write(&mgr, 0, NULL, 1, 10));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_uart_read(&mgr, 0, &data, 0, 10));

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 0, &ops, NULL));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_NO_DEVICE, xy_mux_uart_write(&mgr, 9, &data, 1, 10));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_NO_DEVICE, xy_mux_uart_read(&mgr, 9, &data, 1, 10));

    xy_mux_deinit(&mgr);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uart_register_and_config);
    RUN_TEST(test_uart_write_header_and_payload);
    RUN_TEST(test_uart_read_request_and_data);
    RUN_TEST(test_uart_error_paths);
    return UNITY_END();
}
