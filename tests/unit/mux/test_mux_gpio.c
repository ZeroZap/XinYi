/**
 * @file test_mux_gpio.c
 * @brief Test GPIO Device Operations
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Unit tests for GPIO multiplexing functionality:
 * - GPIO registration
 * - GPIO read/write operations
 * - GPIO configuration
 * - GPIO toggle operations
 */

#include "xy_mux.h"
#include "xy_mux_gpio.h"
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

/* Track mock GPIO state */
static uint8_t g_gpio_levels[16] = {0};

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int32_t, mock_gpio_init, uint8_t, const void *)
FAKE_VALUE_FUNC(int32_t, mock_gpio_deinit, uint8_t)
FAKE_VALUE_FUNC(int32_t, mock_gpio_write, uint8_t, const void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_gpio_read, uint8_t, void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_gpio_ioctl, uint8_t, int, void *)
FAKE_VALUE_FUNC(int32_t, alt_gpio_write, uint8_t, const void *, size_t)
FAKE_VALUE_FUNC(int32_t, alt_gpio_ioctl, uint8_t, int, void *)

/* Mock GPIO operations */
static int32_t mock_gpio_init_impl(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    g_gpio_levels[channel] = 0;
    return XY_MUX_OK;
}

static int32_t mock_gpio_deinit_impl(uint8_t channel)
{
    (void)channel;
    return XY_MUX_OK;
}

static int32_t mock_gpio_write_impl(uint8_t channel, const void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    g_gpio_levels[channel] = ((const uint8_t *)data)[0];
    printf("    [MOCK] GPIO-%d write: %s\n", channel,
           g_gpio_levels[channel] ? "HIGH" : "LOW");
    return sizeof(uint8_t);
}

static int32_t mock_gpio_read_impl(uint8_t channel, void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    ((uint8_t *)data)[0] = g_gpio_levels[channel];
    printf("    [MOCK] GPIO-%d read: %s\n", channel,
           g_gpio_levels[channel] ? "HIGH" : "LOW");
    return sizeof(uint8_t);
}

static int32_t mock_gpio_ioctl_impl(uint8_t channel, int cmd, void *arg)
{
    printf("    [MOCK] GPIO-%d ioctl: cmd=%d\n", channel, cmd);

    switch (cmd) {
        case XY_MUX_GPIO_CMD_SET_LEVEL:
            if (arg) {
                g_gpio_levels[channel] = *(uint8_t *)arg;
                printf("    [MOCK] GPIO-%d set level: %s\n", channel,
                       g_gpio_levels[channel] ? "HIGH" : "LOW");
            }
            break;
        case XY_MUX_GPIO_CMD_GET_LEVEL:
            if (arg) {
                *(uint8_t *)arg = g_gpio_levels[channel];
            }
            break;
        case XY_MUX_GPIO_CMD_TOGGLE:
            g_gpio_levels[channel] = !g_gpio_levels[channel];
            printf("    [MOCK] GPIO-%d toggled: %s\n", channel,
                   g_gpio_levels[channel] ? "HIGH" : "LOW");
            break;
        case XY_MUX_GPIO_CMD_SET_DIR:
            printf("    [MOCK] GPIO-%d set direction: %s\n", channel,
                   arg ? "OUTPUT" : "INPUT");
            break;
        default:
            break;
    }
    return XY_MUX_OK;
}

static int32_t alt_gpio_ioctl_impl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    (void)cmd;
    (void)arg;
    return XY_MUX_OK;
}

static int32_t mock_gpio_init_timeout_impl(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    return XY_MUX_ERROR_TIMEOUT;
}

static int32_t mock_gpio_ioctl_timeout_impl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    (void)cmd;
    (void)arg;
    return XY_MUX_ERROR_TIMEOUT;
}

/* ==================== Test Cases ==================== */

/**
 * @brief Test GPIO registration
 */
static void test_gpio_register(void)
{
    printf("\n[Test] GPIO Registration\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .init = mock_gpio_init,
        .deinit = mock_gpio_deinit,
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };

    /* Register GPIO channels */
    int32_t ret = xy_mux_gpio_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    printf("  [PASS] GPIO-0 registered\n");

    ret = xy_mux_gpio_register(&mgr, 1, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    printf("  [PASS] GPIO-1 registered\n");

    ret = xy_mux_gpio_register(&mgr, 5, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    printf("  [PASS] GPIO-5 registered\n");

    TEST_ASSERT_EQUAL_UINT(3U, mgr.device_count);
    TEST_ASSERT_EQUAL_UINT(3U, mock_gpio_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_init_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(1, mock_gpio_init_fake.arg0_history[1]);
    TEST_ASSERT_EQUAL_UINT8(5, mock_gpio_init_fake.arg0_history[2]);
    printf("  [PASS] Device count = 3\n");

    xy_mux_deinit(&mgr);
    TEST_ASSERT_EQUAL_UINT(3U, mock_gpio_deinit_fake.call_count);
    printf("  [PASS] Deinitialization complete\n");
}

static void test_gpio_register_keeps_per_channel_ops_independent(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t data = XY_MUX_GPIO_HIGH;

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE));

    xy_mux_ops_t primary_ops = {
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_ops_t alt_ops = primary_ops;
    alt_ops.write = alt_gpio_write;
    alt_ops.ioctl = alt_gpio_ioctl;
    alt_gpio_write_fake.return_val = (int32_t)sizeof(data);
    alt_gpio_ioctl_fake.custom_fake = alt_gpio_ioctl_impl;

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_gpio_register(&mgr, 0, &primary_ops, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_gpio_register(&mgr, 1, &alt_ops, NULL));

    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data),
                          xy_mux_write(&mgr, XY_MUX_TYPE_GPIO, 0, &data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, alt_gpio_write_fake.call_count);

    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(data),
                          xy_mux_write(&mgr, XY_MUX_TYPE_GPIO, 1, &data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, alt_gpio_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(1, alt_gpio_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&data, alt_gpio_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), alt_gpio_write_fake.arg2_val);

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_LOW));
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, alt_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_LEVEL, mock_gpio_ioctl_fake.arg1_val);

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_gpio_write(&mgr, 1, XY_MUX_GPIO_HIGH));
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, alt_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(1, alt_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_LEVEL, alt_gpio_ioctl_fake.arg1_val);

    xy_mux_deinit(&mgr);
}

static void test_gpio_register_init_failure_leaves_no_registered_device(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE));

    xy_mux_ops_t ops = {
        .init = mock_gpio_init,
        .deinit = mock_gpio_deinit,
        .ioctl = mock_gpio_ioctl,
    };
    mock_gpio_init_fake.custom_fake = mock_gpio_init_timeout_impl;

    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_TIMEOUT, xy_mux_gpio_register(&mgr, 0, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_gpio_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mgr.device_count);
    TEST_ASSERT_NULL(mgr.devices);

    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_NO_DEVICE,
                          xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_HIGH));
    TEST_ASSERT_EQUAL_UINT(0U, mock_gpio_ioctl_fake.call_count);

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO write operation
 */
static void test_gpio_write(void)
{
    printf("\n[Test] GPIO Write Operation\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Write HIGH */
    int32_t ret = xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_HIGH);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_GPIO_HIGH, g_gpio_levels[0]);
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_LEVEL, mock_gpio_ioctl_fake.arg1_val);
    TEST_ASSERT_NOT_NULL(mock_gpio_ioctl_fake.arg2_val);
    printf("  [PASS] Write HIGH successful\n");

    /* Write LOW */
    ret = xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_LOW);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_GPIO_LOW, g_gpio_levels[0]);
    TEST_ASSERT_EQUAL_UINT(2U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_LEVEL, mock_gpio_ioctl_fake.arg1_val);
    TEST_ASSERT_NOT_NULL(mock_gpio_ioctl_fake.arg2_val);
    printf("  [PASS] Write LOW successful\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO read operation
 */
static void test_gpio_read(void)
{
    printf("\n[Test] GPIO Read Operation\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Set GPIO level via write */
    g_gpio_levels[0] = XY_MUX_GPIO_HIGH;

    /* Read GPIO level */
    int32_t level = xy_mux_gpio_read(&mgr, 0);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_HIGH, level);
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_GET_LEVEL, mock_gpio_ioctl_fake.arg1_val);
    printf("  [PASS] Read HIGH successful\n");

    /* Set GPIO to LOW and read */
    g_gpio_levels[0] = XY_MUX_GPIO_LOW;
    level = xy_mux_gpio_read(&mgr, 0);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_LOW, level);
    TEST_ASSERT_EQUAL_UINT(2U, mock_gpio_ioctl_fake.call_count);
    printf("  [PASS] Read LOW successful\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO toggle operation
 */
static void test_gpio_toggle(void)
{
    printf("\n[Test] GPIO Toggle Operation\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Initial state */
    g_gpio_levels[0] = XY_MUX_GPIO_LOW;

    /* Toggle */
    int32_t ret = xy_mux_gpio_toggle(&mgr, 0);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_GPIO_HIGH, g_gpio_levels[0]);
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_TOGGLE, mock_gpio_ioctl_fake.arg1_val);
    TEST_ASSERT_NULL(mock_gpio_ioctl_fake.arg2_val);
    printf("  [PASS] Toggle to HIGH\n");

    /* Toggle again */
    ret = xy_mux_gpio_toggle(&mgr, 0);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_GPIO_LOW, g_gpio_levels[0]);
    TEST_ASSERT_EQUAL_UINT(2U, mock_gpio_ioctl_fake.call_count);
    printf("  [PASS] Toggle to LOW\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO configuration
 */
static void test_gpio_config(void)
{
    printf("\n[Test] GPIO Configuration\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Configure GPIO as output */
    xy_mux_gpio_config_t config = {
        .dir = XY_MUX_GPIO_OUTPUT,
        .pull = XY_MUX_GPIO_LOW,
        .interrupt_enable = false,
    };

    int32_t ret = xy_mux_gpio_config(&mgr, 0, &config);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_CONFIG, mock_gpio_ioctl_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&config, mock_gpio_ioctl_fake.arg2_val);
    printf("  [PASS] GPIO configured as output\n");

    xy_mux_deinit(&mgr);
}

static void test_gpio_config_propagates_backend_ioctl_failure(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_gpio_config_t config = {
        .dir = XY_MUX_GPIO_OUTPUT,
        .pull = XY_MUX_GPIO_LOW,
        .interrupt_enable = false,
    };

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE));

    xy_mux_ops_t ops = {
        .ioctl = mock_gpio_ioctl,
    };
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_gpio_register(&mgr, 0, &ops, NULL));

    mock_gpio_ioctl_fake.custom_fake = mock_gpio_ioctl_timeout_impl;
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_TIMEOUT, xy_mux_gpio_config(&mgr, 0, &config));
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_CONFIG, mock_gpio_ioctl_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&config, mock_gpio_ioctl_fake.arg2_val);

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO error handling
 */
static void test_gpio_error_handling(void)
{
    printf("\n[Test] GPIO Error Handling\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Try to write to non-registered GPIO */
    int32_t ret = xy_mux_gpio_write(&mgr, 99, XY_MUX_GPIO_HIGH);
    TEST_ASSERT_NOT_EQUAL(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, mock_gpio_ioctl_fake.call_count);
    printf("  [PASS] Write to non-registered GPIO rejected\n");

    /* Try to read from non-registered GPIO */
    ret = xy_mux_gpio_read(&mgr, 99);
    TEST_ASSERT_NOT_EQUAL(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_gpio_read_fake.call_count);
    printf("  [PASS] Read from non-registered GPIO rejected\n");

    /* Try to toggle non-registered GPIO */
    ret = xy_mux_gpio_toggle(&mgr, 99);
    TEST_ASSERT_NOT_EQUAL(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, mock_gpio_ioctl_fake.call_count);
    printf("  [PASS] Toggle non-registered GPIO rejected\n");

    /* Try with NULL manager */
    ret = xy_mux_gpio_write(NULL, 0, XY_MUX_GPIO_HIGH);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL manager rejected\n");

    ret = xy_mux_gpio_read(NULL, 0);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL manager for read rejected\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO multiple channels
 */
static void test_gpio_multi_channel(void)
{
    printf("\n[Test] GPIO Multiple Channels\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .init = mock_gpio_init,
        .deinit = mock_gpio_deinit,
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };

    /* Register 8 GPIO channels */
    for (int ch = 0; ch < 8; ch++) {
        int32_t ret = xy_mux_gpio_register(&mgr, ch, &ops, NULL);
        TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    }
    TEST_ASSERT_EQUAL_UINT(8U, mock_gpio_init_fake.call_count);
    printf("  [PASS] 8 GPIO channels registered\n");

    /* Write to all channels */
    for (int ch = 0; ch < 8; ch++) {
        xy_mux_gpio_level_t level = (ch % 2) ? XY_MUX_GPIO_HIGH : XY_MUX_GPIO_LOW;
        int32_t ret = xy_mux_gpio_write(&mgr, ch, level);
        TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
        TEST_ASSERT_EQUAL_UINT8(level, g_gpio_levels[ch]);
    }
    TEST_ASSERT_EQUAL_UINT(8U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(7, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_SET_LEVEL, mock_gpio_ioctl_fake.arg1_val);
    printf("  [PASS] Write to all 8 channels\n");

    /* Read from all channels */
    for (int ch = 0; ch < 8; ch++) {
        int32_t level = xy_mux_gpio_read(&mgr, ch);
        xy_mux_gpio_level_t expected = (ch % 2) ? XY_MUX_GPIO_HIGH : XY_MUX_GPIO_LOW;
        TEST_ASSERT_EQUAL_INT((int32_t)expected, level);
    }
    TEST_ASSERT_EQUAL_UINT(16U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(7, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_GET_LEVEL, mock_gpio_ioctl_fake.arg1_val);
    printf("  [PASS] Read from all 8 channels\n");

    /* Toggle all channels */
    for (int ch = 0; ch < 8; ch++) {
        xy_mux_gpio_toggle(&mgr, ch);
        xy_mux_gpio_level_t expected = (ch % 2) ? XY_MUX_GPIO_LOW : XY_MUX_GPIO_HIGH;
        TEST_ASSERT_EQUAL_UINT8(expected, g_gpio_levels[ch]);
    }
    TEST_ASSERT_EQUAL_UINT(24U, mock_gpio_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(7, mock_gpio_ioctl_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(XY_MUX_GPIO_CMD_TOGGLE, mock_gpio_ioctl_fake.arg1_val);
    printf("  [PASS] Toggle all 8 channels\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test GPIO TLV packet operations
 */
static void test_gpio_tlv_packet(void)
{
    printf("\n[Test] GPIO TLV Packet Operations\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Build GPIO HIGH packet */
    uint8_t gpio_data = XY_MUX_GPIO_HIGH;
    size_t packet_len = 0;

    int32_t ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_GPIO, 0,
                                      &gpio_data, sizeof(gpio_data),
                                      tx_buffer, &packet_len);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(5U, packet_len); /* 4 byte header + 1 byte data */
    printf("  [PASS] GPIO packet built: %d bytes\n", (int)packet_len);

    /* Verify header structure */
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_TYPE_GPIO, tx_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, tx_buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(1U, tx_buffer[2]); /* length */
    TEST_ASSERT_EQUAL_UINT8(0U, tx_buffer[3]);
    printf("  [PASS] TLV header structure verified\n");

    /* Process the packet */
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(gpio_data), ret);
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_GPIO_HIGH, g_gpio_levels[0]);
    TEST_ASSERT_EQUAL_UINT(1U, mock_gpio_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, mock_gpio_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&tx_buffer[sizeof(xy_mux_header_t)], mock_gpio_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(gpio_data), mock_gpio_write_fake.arg2_val);
    printf("  [PASS] Packet processed, GPIO set to HIGH\n");

    xy_mux_deinit(&mgr);
}

/* ==================== Main Entry ==================== */

void setUp(void)
{
    RESET_FAKE(mock_gpio_init);
    RESET_FAKE(mock_gpio_deinit);
    RESET_FAKE(mock_gpio_write);
    RESET_FAKE(mock_gpio_read);
    RESET_FAKE(mock_gpio_ioctl);
    RESET_FAKE(alt_gpio_write);
    RESET_FAKE(alt_gpio_ioctl);
    FFF_RESET_HISTORY();

    mock_gpio_init_fake.custom_fake = mock_gpio_init_impl;
    mock_gpio_deinit_fake.custom_fake = mock_gpio_deinit_impl;
    mock_gpio_write_fake.custom_fake = mock_gpio_write_impl;
    mock_gpio_read_fake.custom_fake = mock_gpio_read_impl;
    mock_gpio_ioctl_fake.custom_fake = mock_gpio_ioctl_impl;

    memset(g_gpio_levels, 0, sizeof(g_gpio_levels));
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_gpio_register);
    RUN_TEST(test_gpio_register_keeps_per_channel_ops_independent);
    RUN_TEST(test_gpio_register_init_failure_leaves_no_registered_device);
    RUN_TEST(test_gpio_write);
    RUN_TEST(test_gpio_read);
    RUN_TEST(test_gpio_toggle);
    RUN_TEST(test_gpio_config);
    RUN_TEST(test_gpio_config_propagates_backend_ioctl_failure);
    RUN_TEST(test_gpio_error_handling);
    RUN_TEST(test_gpio_multi_channel);
    RUN_TEST(test_gpio_tlv_packet);
    return UNITY_END();
}
