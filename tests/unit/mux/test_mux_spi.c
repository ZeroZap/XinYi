/**
 * @file test_mux_spi.c
 * @brief Focused host tests for MUX SPI helper contracts
 */

#include "xy_mux.h"
#include "xy_mux_spi.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unity.h"
#include "fff.h"

#define BUFFER_SIZE 512U

static uint8_t g_last_write[256];
static size_t g_last_write_len;
static uint8_t g_read_pattern = 0xA5;
static int32_t g_mock_write_forced_ret;
static int32_t g_mock_read_forced_ret;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int32_t, mock_spi_init, uint8_t, const void *)
FAKE_VALUE_FUNC(int32_t, mock_spi_deinit, uint8_t)
FAKE_VALUE_FUNC(int32_t, mock_spi_write, uint8_t, const void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_spi_read, uint8_t, void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_spi_ioctl, uint8_t, int, void *)
FAKE_VALUE_FUNC(int32_t, alt_spi_write, uint8_t, const void *, size_t)

void xy_log_char(char ch)
{
    (void)ch;
}

static int32_t mock_spi_init_impl(uint8_t channel, const void *config)
{
    (void)config;
    printf("    [MOCK] SPI-%u init\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_spi_deinit_impl(uint8_t channel)
{
    printf("    [MOCK] SPI-%u deinit\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_spi_write_impl(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0 || len > sizeof(g_last_write)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    if (g_mock_write_forced_ret != 0) {
        return g_mock_write_forced_ret;
    }
    memcpy(g_last_write, data, len);
    g_last_write_len = len;
    return (int32_t)len;
}

static int32_t mock_spi_read_impl(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    if (g_mock_read_forced_ret != 0) {
        return g_mock_read_forced_ret;
    }
    memset(data, g_read_pattern, len);
    return (int32_t)len;
}

static int32_t mock_spi_ioctl_impl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    if (cmd == XY_MUX_SPI_CMD_SET_CONFIG) {
        TEST_ASSERT_NOT_NULL(arg);
    }
    return XY_MUX_OK;
}

void setUp(void)
{
    RESET_FAKE(mock_spi_init);
    RESET_FAKE(mock_spi_deinit);
    RESET_FAKE(mock_spi_write);
    RESET_FAKE(mock_spi_read);
    RESET_FAKE(mock_spi_ioctl);
    RESET_FAKE(alt_spi_write);
    FFF_RESET_HISTORY();

    mock_spi_init_fake.custom_fake = mock_spi_init_impl;
    mock_spi_deinit_fake.custom_fake = mock_spi_deinit_impl;
    mock_spi_write_fake.custom_fake = mock_spi_write_impl;
    mock_spi_read_fake.custom_fake = mock_spi_read_impl;
    mock_spi_ioctl_fake.custom_fake = mock_spi_ioctl_impl;

    memset(g_last_write, 0, sizeof(g_last_write));
    g_last_write_len = 0;
    g_read_pattern = 0xA5;
    g_mock_write_forced_ret = 0;
    g_mock_read_forced_ret = 0;
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
        .init = mock_spi_init,
        .deinit = mock_spi_deinit,
        .read = mock_spi_read,
        .write = mock_spi_write,
        .ioctl = mock_spi_ioctl,
    };
    return ops;
}

static void test_spi_register_and_config(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 0, &ops, NULL));
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 1, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT8(2, mgr.device_count);
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_spi_init_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(1, mock_spi_init_fake.arg0_history[1]);

    xy_mux_spi_config_t cfg = {
        .speed = 8000000,
        .mode = XY_MUX_SPI_MODE3,
        .bits = 8,
        .cs_pin = 10,
    };
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_config(&mgr, 0, &cfg));
    TEST_ASSERT_EQUAL_UINT(1U, mock_spi_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_spi_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_SPI_CMD_SET_CONFIG, mock_spi_ioctl_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&cfg, mock_spi_ioctl_fake.arg2_val);

    xy_mux_deinit(&mgr);
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_deinit_fake.call_count);
}

static void test_spi_register_keeps_per_channel_ops_independent(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t primary_ops = make_ops();
    xy_mux_ops_t alt_ops = make_ops();
    alt_ops.write = alt_spi_write;
    uint8_t data[] = {0x21, 0x43};

    alt_spi_write_fake.return_val = (int32_t)sizeof(data);

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 0, &primary_ops, NULL));
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 1, &alt_ops, NULL));

    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data), xy_mux_spi_write(&mgr, 0, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, alt_spi_write_fake.call_count);

    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data), xy_mux_spi_write(&mgr, 1, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, alt_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(1, alt_spi_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(data, alt_spi_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), alt_spi_write_fake.arg2_val);

    xy_mux_deinit(&mgr);
}

static void test_spi_write_header_and_payload(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 0, &ops, NULL));

    uint8_t data[] = {0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data), xy_mux_spi_write(&mgr, 0, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_spi_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(data, mock_spi_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), mock_spi_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), g_last_write_len);
    TEST_ASSERT_EQUAL_MEMORY(data, g_last_write, sizeof(data));

    xy_mux_deinit(&mgr);
}

static void test_spi_transfer_and_read(void)
{
    uint8_t tx_buf[BUFFER_SIZE];
    uint8_t rx_buf[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx_buf, rx_buf);
    xy_mux_ops_t ops = make_ops();
    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 2, &ops, NULL));

    uint8_t tx_data[] = {0x9A, 0xBC, 0xDE, 0xF0};
    uint8_t rx_data[sizeof(tx_data)] = {0};
    const uint8_t expected[] = {0xA5, 0xA5, 0xA5, 0xA5};
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(tx_data), xy_mux_spi_transfer(&mgr, 2, tx_data, rx_data, sizeof(tx_data)));
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(2, mock_spi_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(tx_data, mock_spi_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(tx_data), mock_spi_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT(1U, mock_spi_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(2, mock_spi_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(rx_data, mock_spi_read_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(rx_data), mock_spi_read_fake.arg2_val);
    TEST_ASSERT_EQUAL_MEMORY(expected, rx_data, sizeof(rx_data));

    memset(rx_data, 0, sizeof(rx_data));
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(rx_data), xy_mux_spi_read(&mgr, 2, rx_data, sizeof(rx_data)));
    TEST_ASSERT_EQUAL_UINT(4U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, mock_spi_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0xA5, rx_data[0]);

    xy_mux_deinit(&mgr);
}

static void test_spi_error_paths(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    uint8_t data = 0x55;

    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_spi_write(NULL, 0, &data, 1));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_spi_read(NULL, 0, &data, 1));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_spi_transfer(NULL, 0, &data, &data, 1));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_INVALID_PARAM, xy_mux_spi_transfer(&mgr, 0, &data, &data, 0));

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 0, &ops, NULL));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_NO_DEVICE, xy_mux_spi_write(&mgr, 9, &data, 1));
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_NO_DEVICE, xy_mux_spi_read(&mgr, 9, &data, 1));
    TEST_ASSERT_EQUAL_UINT(0U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_spi_read_fake.call_count);

    xy_mux_deinit(&mgr);
}

static void test_spi_transfer_preserves_rx_on_failed_write_or_read(void)
{
    uint8_t tx_buf[BUFFER_SIZE];
    uint8_t rx_buf[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx_buf, rx_buf);
    xy_mux_ops_t ops = make_ops();
    uint8_t tx_data[] = {0x11, 0x22, 0x33};
    uint8_t rx_data[] = {0xA1, 0xB2, 0xC3};

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 3, &ops, NULL));

    g_mock_write_forced_ret = XY_MUX_ERROR_NO_MEMORY;
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_NO_MEMORY,
                      xy_mux_spi_transfer(&mgr, 3, tx_data, rx_data, sizeof(tx_data)));
    TEST_ASSERT_EQUAL_HEX8(0xA1, rx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xB2, rx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xC3, rx_data[2]);
    TEST_ASSERT_EQUAL_UINT(1U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_spi_read_fake.call_count);

    g_mock_write_forced_ret = 0;
    g_mock_read_forced_ret = XY_MUX_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_TIMEOUT,
                      xy_mux_spi_transfer(&mgr, 3, tx_data, rx_data, sizeof(tx_data)));
    TEST_ASSERT_EQUAL_HEX8(0xA1, rx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xB2, rx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xC3, rx_data[2]);
    TEST_ASSERT_EQUAL_UINT(3U, mock_spi_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, mock_spi_read_fake.call_count);

    xy_mux_deinit(&mgr);
}

static void test_spi_read_preserves_output_on_transfer_failure(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    uint8_t data[] = {0x5A, 0x6B, 0x7C, 0x8D};

    TEST_ASSERT_EQUAL(XY_MUX_OK, xy_mux_spi_register(&mgr, 4, &ops, NULL));

    g_mock_read_forced_ret = XY_MUX_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL(XY_MUX_ERROR_TIMEOUT, xy_mux_spi_read(&mgr, 4, data, sizeof(data)));
    TEST_ASSERT_EQUAL_HEX8(0x5A, data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x6B, data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x7C, data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x8D, data[3]);

    xy_mux_deinit(&mgr);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_spi_register_and_config);
    RUN_TEST(test_spi_register_keeps_per_channel_ops_independent);
    RUN_TEST(test_spi_write_header_and_payload);
    RUN_TEST(test_spi_transfer_and_read);
    RUN_TEST(test_spi_error_paths);
    RUN_TEST(test_spi_transfer_preserves_rx_on_failed_write_or_read);
    RUN_TEST(test_spi_read_preserves_output_on_transfer_failure);
    return UNITY_END();
}
