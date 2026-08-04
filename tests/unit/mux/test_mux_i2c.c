/**
 * @file test_mux_i2c.c
 * @brief Test I2C Device Operations
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Unit tests for I2C bus multiplexing functionality:
 * - I2C registration
 * - I2C read/write operations
 * - I2C transfer with messages
 * - I2C device scanning
 */

#include "xy_mux.h"
#include "xy_mux_i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity.h"
#include "fff.h"

void xy_log_char(char ch)
{
    (void)ch;
}

/* Test buffer size */
#define BUFFER_SIZE 512

/* Track mock I2C state */
static uint8_t g_i2c_buffers[16][256];
static size_t g_i2c_buffer_lens[16];
static int32_t g_mock_i2c_write_forced_ret;
static int32_t g_mock_i2c_read_forced_ret;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int32_t, mock_i2c_init, uint8_t, const void *)
FAKE_VALUE_FUNC(int32_t, mock_i2c_deinit, uint8_t)
FAKE_VALUE_FUNC(int32_t, mock_i2c_write, uint8_t, const void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_i2c_read, uint8_t, void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_i2c_ioctl, uint8_t, int, void *)
FAKE_VALUE_FUNC(int32_t, alt_i2c_write, uint8_t, const void *, size_t)

/* Mock I2C operations */
static int32_t mock_i2c_init_impl(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    memset(g_i2c_buffers[channel], 0, sizeof(g_i2c_buffers[channel]));
    g_i2c_buffer_lens[channel] = 0;
    return XY_MUX_OK;
}

static int32_t mock_i2c_deinit_impl(uint8_t channel)
{
    (void)channel;
    return XY_MUX_OK;
}

static int32_t mock_i2c_write_impl(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0 || len > 256) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    if (g_mock_i2c_write_forced_ret != 0) {
        return g_mock_i2c_write_forced_ret;
    }
    memcpy(g_i2c_buffers[channel], data, len);
    g_i2c_buffer_lens[channel] = len;
    printf("    [MOCK] I2C-%d write %d bytes\n", channel, (int)len);
    return (int32_t)len;
}

static int32_t mock_i2c_read_impl(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    if (g_mock_i2c_read_forced_ret != 0) {
        return g_mock_i2c_read_forced_ret;
    }
    /* Return simulated data */
    memset(data, 0xA5, len);
    printf("    [MOCK] I2C-%d read %d bytes (simulated)\n", channel, (int)len);
    return (int32_t)len;
}

static int32_t short_i2c_write_impl(uint8_t channel, const void *data, size_t len)
{
    int32_t ret = mock_i2c_write_impl(channel, data, len);
    if (ret > 1) {
        return ret - 1;
    }
    return ret;
}

static int32_t short_i2c_read_impl(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    if (len > 1U) {
        memset(data, 0xA5, len - 1U);
        return (int32_t)(len - 1U);
    }
    return mock_i2c_read_impl(channel, data, len);
}

static int32_t mock_i2c_ioctl_impl(uint8_t channel, int cmd, void *arg)
{
    printf("    [MOCK] I2C-%d ioctl: cmd=%d\n", channel, cmd);

    switch (cmd) {
        case XY_MUX_I2C_CMD_SET_SPEED:
            if (arg) {
                uint32_t speed = *(uint32_t *)arg;
                printf("    [MOCK] I2C-%d set speed: %lu Hz\n", channel, (unsigned long)speed);
            }
            break;
        case XY_MUX_I2C_CMD_GET_SPEED:
            if (arg) {
                *(uint32_t *)arg = 400000;
            }
            break;
        case XY_MUX_I2C_CMD_SCAN:
            printf("    [MOCK] I2C-%d scan requested\n", channel);
            break;
        default:
            break;
    }
    return XY_MUX_OK;
}

/* ==================== Test Cases ==================== */

/**
 * @brief Test I2C registration
 */
static void test_i2c_register(void)
{
    printf("\n[Test] I2C Registration\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .init = mock_i2c_init,
        .deinit = mock_i2c_deinit,
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };

    /* Register I2C channel 0 */
    int32_t ret = xy_mux_i2c_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    printf("  [PASS] I2C-0 registered\n");

    /* Register I2C channel 1 */
    ret = xy_mux_i2c_register(&mgr, 1, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    printf("  [PASS] I2C-1 registered\n");

    /* Register I2C channel 2 */
    ret = xy_mux_i2c_register(&mgr, 2, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    printf("  [PASS] I2C-2 registered\n");

    TEST_ASSERT_EQUAL_UINT(3U, mgr.device_count);
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_init_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(1, mock_i2c_init_fake.arg0_history[1]);
    TEST_ASSERT_EQUAL_UINT8(2, mock_i2c_init_fake.arg0_history[2]);
    printf("  [PASS] Device count = 3\n");

    xy_mux_deinit(&mgr);
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_deinit_fake.call_count);
}

static void test_i2c_register_keeps_per_channel_ops_independent(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t data[] = {0x5A};

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE));

    xy_mux_ops_t primary_ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_ops_t alt_ops = primary_ops;
    alt_ops.write = alt_i2c_write;
    alt_i2c_write_fake.return_val = 3;

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 0, &primary_ops, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 1, &alt_ops, NULL));

    TEST_ASSERT_EQUAL_INT(3, xy_mux_i2c_write(&mgr, 0, 0x50, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, alt_i2c_write_fake.call_count);

    TEST_ASSERT_EQUAL_INT(3, xy_mux_i2c_write(&mgr, 1, 0x60, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, alt_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(1, alt_i2c_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(3U, alt_i2c_write_fake.arg2_val);

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C configuration
 */
static void test_i2c_config(void)
{
    printf("\n[Test] I2C Configuration\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);

    /* Configure I2C bus */
    xy_mux_i2c_config_t config = {
        .speed = 400000,
        .addr_bits = 7,
    };

    int32_t ret = xy_mux_i2c_config(&mgr, 0, &config);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_I2C_CMD_SET_CONFIG, mock_i2c_ioctl_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&config, mock_i2c_ioctl_fake.arg2_val);
    printf("  [PASS] I2C configured at 400kHz, 7-bit addressing\n");

    /* Configure with 10-bit addressing */
    config.addr_bits = 10;
    ret = xy_mux_i2c_config(&mgr, 0, &config);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(2U, mock_i2c_ioctl_fake.call_count);
    printf("  [PASS] I2C reconfigured with 10-bit addressing\n");

    /* Configure high speed */
    config.speed = 1000000;
    config.addr_bits = 7;
    ret = xy_mux_i2c_config(&mgr, 0, &config);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_ioctl_fake.call_count);
    printf("  [PASS] I2C configured at 1MHz\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C write operation
 */
static void test_i2c_write(void)
{
    printf("\n[Test] I2C Write Operation\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);

    /* Write to I2C device */
    uint8_t data[] = {0x55, 0xAA, 0x01, 0x02};
    int32_t ret = xy_mux_i2c_write(&mgr, 0, 0x50, data, sizeof(data));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data) + 2U, mock_i2c_write_fake.arg2_val);
    printf("  [PASS] Write %d bytes to I2C device at address 0x50\n", (int)sizeof(data));

    /* Verify data was written to mock buffer */
    TEST_ASSERT_EQUAL_UINT(sizeof(data) + 2U, g_i2c_buffer_lens[0]); /* +2 for address bytes */
    printf("  [PASS] Data verified in mock buffer\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C read operation
 */
static void test_i2c_read(void)
{
    printf("\n[Test] I2C Read Operation\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);

    /* Read from I2C device */
    uint8_t buffer[16];
    int32_t ret = xy_mux_i2c_read(&mgr, 0, 0x50, buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(buffer, mock_i2c_read_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(buffer), mock_i2c_read_fake.arg2_val);
    printf("  [PASS] Read %d bytes from I2C device\n", ret);

    /* Verify buffer contains simulated data */
    TEST_ASSERT_EQUAL_UINT8(0xA5U, buffer[0]);
    printf("  [PASS] Buffer contains expected pattern\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C transfer with messages
 */
static void test_i2c_transfer(void)
{
    printf("\n[Test] I2C Transfer with Messages\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);

    /* Prepare I2C messages for a typical register read */
    uint8_t reg_addr = 0x10;
    uint8_t data_out[] = {0x11, 0x22, 0x33};
    uint8_t data_in[8];

    xy_mux_i2c_msg_t msgs[] = {
        {0x50, 0, 1, &reg_addr},           /* Write register address */
        {0x50, 0, 3, data_out},            /* Write data */
        {0x50, XY_MUX_I2C_M_RD, 8, data_in}, /* Read response */
    };

    /* Perform transfer */
    int32_t ret = xy_mux_i2c_transfer(&mgr, 0, msgs, 3);
    TEST_ASSERT_EQUAL_INT(16, ret);
    TEST_ASSERT_EQUAL_UINT(2U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_write_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_write_fake.arg2_history[0]);
    TEST_ASSERT_EQUAL_UINT(5U, mock_i2c_write_fake.arg2_history[1]);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(data_in, mock_i2c_read_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data_in), mock_i2c_read_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, data_in[0]);
    printf("  [PASS] Multi-message transfer completed\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C error handling
 */
static void test_i2c_error_handling(void)
{
    printf("\n[Test] I2C Error Handling\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);

    /* Try to read from non-registered channel */
    uint8_t buffer[16];
    int32_t ret = xy_mux_i2c_read(&mgr, 99, 0x50, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_read_fake.call_count);
    printf("  [PASS] Read from non-registered channel rejected\n");

    /* Try to write to non-registered channel */
    uint8_t data[] = {0x55};
    ret = xy_mux_i2c_write(&mgr, 99, 0x50, data, sizeof(data));
    TEST_ASSERT_NOT_EQUAL(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_write_fake.call_count);
    printf("  [PASS] Write to non-registered channel rejected\n");

    /* Try with NULL manager */
    ret = xy_mux_i2c_write(NULL, 0, 0x50, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL manager rejected for write\n");

    ret = xy_mux_i2c_read(NULL, 0, 0x50, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL manager rejected for read\n");

    /* Try with NULL data */
    ret = xy_mux_i2c_write(&mgr, 0, 0x50, NULL, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL data rejected for write\n");

    xy_mux_deinit(&mgr);
}

static void test_i2c_read_preserves_output_when_request_write_fails(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t buffer[] = {0x11, 0x22, 0x33, 0x44};

    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 0, &ops, NULL));

    g_mock_i2c_write_forced_ret = (int32_t)XY_MUX_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT((int32_t)XY_MUX_ERROR_TIMEOUT,
                          xy_mux_i2c_read(&mgr, 0, 0x50, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0x11, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0x33, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x44, buffer[3]);

    xy_mux_deinit(&mgr);
}

static void test_i2c_transfer_stops_on_failed_message_and_preserves_later_read(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t write_data[] = {0x10};
    uint8_t read_data[] = {0xAA, 0xBB, 0xCC};

    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 0, &ops, NULL));

    xy_mux_i2c_msg_t msgs[] = {
        {0x50, 0, sizeof(write_data), write_data},
        {0x50, XY_MUX_I2C_M_RD, sizeof(read_data), read_data},
    };

    g_mock_i2c_write_forced_ret = (int32_t)XY_MUX_ERROR_NO_DEVICE;
    TEST_ASSERT_EQUAL_INT((int32_t)XY_MUX_ERROR_NO_DEVICE, xy_mux_i2c_transfer(&mgr, 0, msgs, 2));
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0xAA, read_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, read_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, read_data[2]);

    xy_mux_deinit(&mgr);
}

static void test_i2c_read_propagates_short_request_write_without_reading(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t buffer[] = {0xDE, 0xAD};

    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 0, &ops, NULL));

    mock_i2c_write_fake.custom_fake = short_i2c_write_impl;
    TEST_ASSERT_EQUAL_INT(2, xy_mux_i2c_read(&mgr, 0, 0x50, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0xDE, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, buffer[1]);

    xy_mux_deinit(&mgr);
}

static void test_i2c_transfer_propagates_short_write_and_stops(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t write_data[] = {0x10, 0x20};
    uint8_t read_data[] = {0xAA, 0xBB};

    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 0, &ops, NULL));

    xy_mux_i2c_msg_t msgs[] = {
        {0x50, 0, sizeof(write_data), write_data},
        {0x50, XY_MUX_I2C_M_RD, sizeof(read_data), read_data},
    };

    mock_i2c_write_fake.custom_fake = short_i2c_write_impl;
    TEST_ASSERT_EQUAL_INT(3, xy_mux_i2c_transfer(&mgr, 0, msgs, 2));
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0xAA, read_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, read_data[1]);

    xy_mux_deinit(&mgr);
}

static void test_i2c_transfer_propagates_short_read_and_preserves_tail(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t read_data[] = {0xAA, 0xBB, 0xCC};

    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_i2c_register(&mgr, 0, &ops, NULL));

    xy_mux_i2c_msg_t msg = {0x50, XY_MUX_I2C_M_RD, sizeof(read_data), read_data};

    mock_i2c_read_fake.custom_fake = short_i2c_read_impl;
    TEST_ASSERT_EQUAL_INT(2, xy_mux_i2c_transfer(&mgr, 0, &msg, 1));
    TEST_ASSERT_EQUAL_UINT(0U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0xA5, read_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xA5, read_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, read_data[2]);

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C TLV packet operations
 */
static void test_i2c_tlv_packet(void)
{
    printf("\n[Test] I2C TLV Packet Operations\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);

    /* Build I2C write packet */
    uint8_t i2c_data[] = {0x50, 0x00, 0x55, 0xAA}; /* addr + data */
    size_t packet_len = 0;

    int32_t ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_I2C, 0,
                                      i2c_data, sizeof(i2c_data),
                                      tx_buffer, &packet_len);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(sizeof(xy_mux_header_t) + sizeof(i2c_data), packet_len);
    printf("  [PASS] I2C packet built: %d bytes\n", (int)packet_len);

    /* Verify header structure */
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_TYPE_I2C, tx_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, tx_buffer[1]);  /* channel */
    TEST_ASSERT_EQUAL_UINT8(sizeof(i2c_data), tx_buffer[2]); /* length */
    printf("  [PASS] TLV header structure verified\n");

    /* Process the packet */
    g_i2c_buffer_lens[0] = 0;
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(i2c_data), ret);
    TEST_ASSERT_EQUAL_UINT(1U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_MEMORY(i2c_data, mock_i2c_write_fake.arg1_val, sizeof(i2c_data));
    TEST_ASSERT_EQUAL_UINT(sizeof(i2c_data), mock_i2c_write_fake.arg2_val);
    printf("  [PASS] Packet processed\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test I2C multiple buses
 */
static void test_i2c_multi_bus(void)
{
    printf("\n[Test] I2C Multiple Buses\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_i2c_read,
        .write = mock_i2c_write,
        .ioctl = mock_i2c_ioctl,
    };

    /* Register three I2C buses with different configurations */
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);
    xy_mux_i2c_config_t config0 = {.speed = 400000, .addr_bits = 7};
    xy_mux_i2c_config(&mgr, 0, &config0);

    xy_mux_i2c_register(&mgr, 1, &ops, NULL);
    xy_mux_i2c_config_t config1 = {.speed = 100000, .addr_bits = 7};
    xy_mux_i2c_config(&mgr, 1, &config1);

    xy_mux_i2c_register(&mgr, 2, &ops, NULL);
    xy_mux_i2c_config_t config2 = {.speed = 1000000, .addr_bits = 10};
    xy_mux_i2c_config(&mgr, 2, &config2);

    printf("  [PASS] Three I2C buses registered with different configs\n");

    /* Write to each bus */
    uint8_t data = 0xAB;
    xy_mux_i2c_write(&mgr, 0, 0x50, &data, 1);
    printf("  [PASS] Write to I2C-0 (400kHz)\n");

    xy_mux_i2c_write(&mgr, 1, 0x30, &data, 1);
    printf("  [PASS] Write to I2C-1 (100kHz)\n");

    xy_mux_i2c_write(&mgr, 2, 0x40, &data, 1);
    printf("  [PASS] Write to I2C-2 (1MHz, 10-bit)\n");
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_i2c_write_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(1, mock_i2c_write_fake.arg0_history[1]);
    TEST_ASSERT_EQUAL_UINT8(2, mock_i2c_write_fake.arg0_history[2]);

    /* Read from each bus */
    uint8_t buffer[4];
    xy_mux_i2c_read(&mgr, 0, 0x50, buffer, 1);
    printf("  [PASS] Read from I2C-0\n");

    xy_mux_i2c_read(&mgr, 1, 0x30, buffer, 1);
    printf("  [PASS] Read from I2C-1\n");

    xy_mux_i2c_read(&mgr, 2, 0x40, buffer, 1);
    printf("  [PASS] Read from I2C-2\n");
    TEST_ASSERT_EQUAL_UINT(6U, mock_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(3U, mock_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(2, mock_i2c_read_fake.arg0_val);

    xy_mux_deinit(&mgr);
}

/* ==================== Main Entry ==================== */

void setUp(void)
{
    RESET_FAKE(mock_i2c_init);
    RESET_FAKE(mock_i2c_deinit);
    RESET_FAKE(mock_i2c_write);
    RESET_FAKE(mock_i2c_read);
    RESET_FAKE(mock_i2c_ioctl);
    RESET_FAKE(alt_i2c_write);
    FFF_RESET_HISTORY();

    mock_i2c_init_fake.custom_fake = mock_i2c_init_impl;
    mock_i2c_deinit_fake.custom_fake = mock_i2c_deinit_impl;
    mock_i2c_write_fake.custom_fake = mock_i2c_write_impl;
    mock_i2c_read_fake.custom_fake = mock_i2c_read_impl;
    mock_i2c_ioctl_fake.custom_fake = mock_i2c_ioctl_impl;

    memset(g_i2c_buffers, 0, sizeof(g_i2c_buffers));
    memset(g_i2c_buffer_lens, 0, sizeof(g_i2c_buffer_lens));
    g_mock_i2c_write_forced_ret = 0;
    g_mock_i2c_read_forced_ret = 0;
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_i2c_register);
    RUN_TEST(test_i2c_register_keeps_per_channel_ops_independent);
    RUN_TEST(test_i2c_config);
    RUN_TEST(test_i2c_write);
    RUN_TEST(test_i2c_read);
    RUN_TEST(test_i2c_transfer);
    RUN_TEST(test_i2c_error_handling);
    RUN_TEST(test_i2c_read_preserves_output_when_request_write_fails);
    RUN_TEST(test_i2c_transfer_stops_on_failed_message_and_preserves_later_read);
    RUN_TEST(test_i2c_read_propagates_short_request_write_without_reading);
    RUN_TEST(test_i2c_transfer_propagates_short_write_and_stops);
    RUN_TEST(test_i2c_transfer_propagates_short_read_and_preserves_tail);
    RUN_TEST(test_i2c_tlv_packet);
    RUN_TEST(test_i2c_multi_bus);
    return UNITY_END();
}
