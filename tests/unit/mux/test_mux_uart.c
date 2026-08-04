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
#include "fff.h"

#define BUFFER_SIZE 512U

static uint8_t g_last_write[256];
static size_t g_last_write_len;
static uint8_t g_read_pattern = 0x5A;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int32_t, mock_uart_init, uint8_t, const void *)
FAKE_VALUE_FUNC(int32_t, mock_uart_deinit, uint8_t)
FAKE_VALUE_FUNC(int32_t, mock_uart_write, uint8_t, const void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_uart_read, uint8_t, void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_uart_ioctl, uint8_t, int, void *)

void xy_log_char(char ch)
{
    (void)ch;
}

static int32_t mock_uart_init_impl(uint8_t channel, const void *config)
{
    (void)config;
    printf("    [MOCK] UART-%u init\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_uart_deinit_impl(uint8_t channel)
{
    printf("    [MOCK] UART-%u deinit\n", (unsigned)channel);
    return XY_MUX_OK;
}

static uint8_t g_write_history[4][256];
static size_t g_write_len_history[4];

static int32_t mock_uart_write_impl(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0 || len > sizeof(g_last_write)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memcpy(g_last_write, data, len);
    g_last_write_len = len;
    if (mock_uart_write_fake.call_count > 0U && mock_uart_write_fake.call_count <= 4U) {
        size_t index = mock_uart_write_fake.call_count - 1U;
        memcpy(g_write_history[index], data, len);
        g_write_len_history[index] = len;
    }
    return (int32_t)len;
}

static int32_t mock_uart_read_impl(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memset(data, g_read_pattern, len);
    return (int32_t)len;
}

static int32_t mock_uart_ioctl_impl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    if (cmd == XY_MUX_UART_CMD_SET_CONFIG) {
        TEST_ASSERT_NOT_NULL(arg);
    }
    return XY_MUX_OK;
}

void setUp(void)
{
    RESET_FAKE(mock_uart_init);
    RESET_FAKE(mock_uart_deinit);
    RESET_FAKE(mock_uart_write);
    RESET_FAKE(mock_uart_read);
    RESET_FAKE(mock_uart_ioctl);
    FFF_RESET_HISTORY();

    mock_uart_init_fake.custom_fake = mock_uart_init_impl;
    mock_uart_deinit_fake.custom_fake = mock_uart_deinit_impl;
    mock_uart_write_fake.custom_fake = mock_uart_write_impl;
    mock_uart_read_fake.custom_fake = mock_uart_read_impl;
    mock_uart_ioctl_fake.custom_fake = mock_uart_ioctl_impl;

    memset(g_last_write, 0, sizeof(g_last_write));
    g_last_write_len = 0;
    memset(g_write_history, 0, sizeof(g_write_history));
    memset(g_write_len_history, 0, sizeof(g_write_len_history));
    g_read_pattern = 0x5A;
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
    TEST_ASSERT_EQUAL_UINT(2U, mock_uart_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_uart_init_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(1, mock_uart_init_fake.arg0_history[1]);

    xy_mux_uart_config_t cfg = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
        .flow_control = 0,
    };
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_config(&mgr, 0, &cfg));
    TEST_ASSERT_EQUAL_UINT(1U, mock_uart_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_uart_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_UART_CMD_SET_CONFIG, mock_uart_ioctl_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&cfg, mock_uart_ioctl_fake.arg2_val);

    xy_mux_deinit(&mgr);
    TEST_ASSERT_EQUAL_UINT(2U, mock_uart_deinit_fake.call_count);
}

static void assert_uart_request_header(const uint8_t *header, size_t expected_len, uint32_t timeout)
{
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(expected_len & 0xFFU), header[0]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)((expected_len >> 8) & 0xFFU), header[1]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)((expected_len >> 16) & 0xFFU), header[2]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)((expected_len >> 24) & 0xFFU), header[3]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(timeout & 0xFFU), header[4]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)((timeout >> 8) & 0xFFU), header[5]);
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
    TEST_ASSERT_EQUAL_UINT(2U, mock_uart_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_uart_write_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(0, mock_uart_write_fake.arg0_history[1]);
    TEST_ASSERT_EQUAL_UINT(6U, g_write_len_history[0]);
    assert_uart_request_header(g_write_history[0], sizeof(data), 250U);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), g_write_len_history[1]);
    TEST_ASSERT_EQUAL_MEMORY(data, g_write_history[1], sizeof(data));
    TEST_ASSERT_EQUAL_PTR(data, mock_uart_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), mock_uart_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), g_last_write_len);
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
    TEST_ASSERT_EQUAL_UINT(1U, mock_uart_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(2, mock_uart_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(6U, mock_uart_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT(6U, g_write_len_history[0]);
    assert_uart_request_header(g_write_history[0], sizeof(data), 1000U);
    TEST_ASSERT_EQUAL_UINT(1U, mock_uart_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(2, mock_uart_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(data, mock_uart_read_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), mock_uart_read_fake.arg2_val);
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
    TEST_ASSERT_EQUAL_UINT(0U, mock_uart_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_uart_read_fake.call_count);

    xy_mux_deinit(&mgr);
}

static int32_t fail_after_header_write_impl(uint8_t channel, const void *data, size_t len)
{
    if (mock_uart_write_fake.call_count == 1U) {
        return mock_uart_write_impl(channel, data, len);
    }
    return XY_MUX_ERROR_TIMEOUT;
}

static int32_t short_header_write_impl(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    (void)data;
    (void)len;
    return 3;
}

static int32_t short_payload_write_impl(uint8_t channel, const void *data, size_t len)
{
    if (mock_uart_write_fake.call_count == 1U) {
        return mock_uart_write_impl(channel, data, len);
    }
    return (int32_t)(len - 1U);
}

static int32_t short_read_impl(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(2U, len);
    ((uint8_t *)data)[0] = 0xA5;
    ((uint8_t *)data)[1] = 0x5A;
    return 2;
}

static void test_uart_write_stops_when_payload_write_fails(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    const uint8_t data[] = {0x10, 0x20, 0x30};

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 0, &ops, NULL));
    mock_uart_write_fake.custom_fake = fail_after_header_write_impl;

    TEST_ASSERT_EQUAL(XY_MUX_ERROR_TIMEOUT, xy_mux_uart_write(&mgr, 0, data, sizeof(data), 25));
    TEST_ASSERT_EQUAL_UINT(2U, mock_uart_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(6U, g_write_len_history[0]);
    assert_uart_request_header(g_write_history[0], sizeof(data), 25U);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_len_history[1]);

    xy_mux_deinit(&mgr);
}

static void test_uart_write_returns_short_header_result_without_payload(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    const uint8_t data[] = {0xAA, 0xBB};

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 1, &ops, NULL));
    mock_uart_write_fake.custom_fake = short_header_write_impl;

    TEST_ASSERT_EQUAL_INT(3, xy_mux_uart_write(&mgr, 1, data, sizeof(data), 80));
    TEST_ASSERT_EQUAL_UINT(1U, mock_uart_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(1, mock_uart_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(6U, mock_uart_write_fake.arg2_val);

    xy_mux_deinit(&mgr);
}

static void test_uart_write_returns_short_payload_result(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 1, &ops, NULL));
    mock_uart_write_fake.custom_fake = short_payload_write_impl;

    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data) - 1,
                          xy_mux_uart_write(&mgr, 1, data, sizeof(data), 80));
    TEST_ASSERT_EQUAL_UINT(2U, mock_uart_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(6U, g_write_len_history[0]);
    assert_uart_request_header(g_write_history[0], sizeof(data), 80U);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_len_history[1]);

    xy_mux_deinit(&mgr);
}

static void test_uart_read_preserves_tail_when_backend_short_reads(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_uart_register(&mgr, 2, &ops, NULL));
    mock_uart_read_fake.custom_fake = short_read_impl;

    TEST_ASSERT_EQUAL_INT(2, xy_mux_uart_read(&mgr, 2, data, sizeof(data), 10));
    TEST_ASSERT_EQUAL_UINT(1U, mock_uart_write_fake.call_count);
    assert_uart_request_header(g_write_history[0], sizeof(data), 10U);
    TEST_ASSERT_EQUAL_UINT(1U, mock_uart_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0xA5, data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x5A, data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x33, data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x44, data[3]);
    TEST_ASSERT_EQUAL_HEX8(0x55, data[4]);

    xy_mux_deinit(&mgr);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uart_register_and_config);
    RUN_TEST(test_uart_write_header_and_payload);
    RUN_TEST(test_uart_read_request_and_data);
    RUN_TEST(test_uart_error_paths);
    RUN_TEST(test_uart_write_stops_when_payload_write_fails);
    RUN_TEST(test_uart_write_returns_short_header_result_without_payload);
    RUN_TEST(test_uart_write_returns_short_payload_result);
    RUN_TEST(test_uart_read_preserves_tail_when_backend_short_reads);
    return UNITY_END();
}
