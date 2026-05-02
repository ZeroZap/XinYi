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
#include <assert.h>

/* Test buffer size */
#define BUFFER_SIZE 512

/* Track mock GPIO state */
static uint8_t g_gpio_levels[16] = {0};
static int g_gpio_write_count = 0;

/* Mock GPIO operations */
static int32_t mock_gpio_init(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    g_gpio_levels[channel] = 0;
    return XY_MUX_OK;
}

static int32_t mock_gpio_deinit(uint8_t channel)
{
    (void)channel;
    return XY_MUX_OK;
}

static int32_t mock_gpio_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    g_gpio_levels[channel] = ((const uint8_t *)data)[0];
    g_gpio_write_count++;
    printf("    [MOCK] GPIO-%d write: %s\n", channel,
           g_gpio_levels[channel] ? "HIGH" : "LOW");
    return sizeof(uint8_t);
}

static int32_t mock_gpio_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    ((uint8_t *)data)[0] = g_gpio_levels[channel];
    printf("    [MOCK] GPIO-%d read: %s\n", channel,
           g_gpio_levels[channel] ? "HIGH" : "LOW");
    return sizeof(uint8_t);
}

static int32_t mock_gpio_ioctl(uint8_t channel, int cmd, void *arg)
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
    assert(ret == XY_MUX_OK);
    printf("  [PASS] GPIO-0 registered\n");

    ret = xy_mux_gpio_register(&mgr, 1, &ops, NULL);
    assert(ret == XY_MUX_OK);
    printf("  [PASS] GPIO-1 registered\n");

    ret = xy_mux_gpio_register(&mgr, 5, &ops, NULL);
    assert(ret == XY_MUX_OK);
    printf("  [PASS] GPIO-5 registered\n");

    assert(mgr.device_count == 3);
    printf("  [PASS] Device count = 3\n");

    xy_mux_deinit(&mgr);
    printf("  [PASS] Deinitialization complete\n");
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

    g_gpio_write_count = 0;

    /* Write HIGH */
    int32_t ret = xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_HIGH);
    assert(ret == XY_MUX_OK);
    assert(g_gpio_levels[0] == XY_MUX_GPIO_HIGH);
    printf("  [PASS] Write HIGH successful\n");

    /* Write LOW */
    ret = xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_LOW);
    assert(ret == XY_MUX_OK);
    assert(g_gpio_levels[0] == XY_MUX_GPIO_LOW);
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
    assert(level == XY_MUX_GPIO_HIGH);
    printf("  [PASS] Read HIGH successful\n");

    /* Set GPIO to LOW and read */
    g_gpio_levels[0] = XY_MUX_GPIO_LOW;
    level = xy_mux_gpio_read(&mgr, 0);
    assert(level == XY_MUX_GPIO_LOW);
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
    assert(ret == XY_MUX_OK);
    assert(g_gpio_levels[0] == XY_MUX_GPIO_HIGH);
    printf("  [PASS] Toggle to HIGH\n");

    /* Toggle again */
    ret = xy_mux_gpio_toggle(&mgr, 0);
    assert(ret == XY_MUX_OK);
    assert(g_gpio_levels[0] == XY_MUX_GPIO_LOW);
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
    assert(ret == XY_MUX_OK);
    printf("  [PASS] GPIO configured as output\n");

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
    assert(ret != XY_MUX_OK);
    printf("  [PASS] Write to non-registered GPIO rejected\n");

    /* Try to read from non-registered GPIO */
    ret = xy_mux_gpio_read(&mgr, 99);
    assert(ret != XY_MUX_OK);
    printf("  [PASS] Read from non-registered GPIO rejected\n");

    /* Try to toggle non-registered GPIO */
    ret = xy_mux_gpio_toggle(&mgr, 99);
    assert(ret != XY_MUX_OK);
    printf("  [PASS] Toggle non-registered GPIO rejected\n");

    /* Try with NULL manager */
    ret = xy_mux_gpio_write(NULL, 0, XY_MUX_GPIO_HIGH);
    assert(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL manager rejected\n");

    ret = xy_mux_gpio_read(NULL, 0);
    assert(ret == XY_MUX_ERROR_INVALID_PARAM);
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
        .read = mock_gpio_read,
        .write = mock_gpio_write,
        .ioctl = mock_gpio_ioctl,
    };

    /* Register 8 GPIO channels */
    for (int ch = 0; ch < 8; ch++) {
        int32_t ret = xy_mux_gpio_register(&mgr, ch, &ops, NULL);
        assert(ret == XY_MUX_OK);
    }
    printf("  [PASS] 8 GPIO channels registered\n");

    /* Write to all channels */
    for (int ch = 0; ch < 8; ch++) {
        xy_mux_gpio_level_t level = (ch % 2) ? XY_MUX_GPIO_HIGH : XY_MUX_GPIO_LOW;
        int32_t ret = xy_mux_gpio_write(&mgr, ch, level);
        assert(ret == XY_MUX_OK);
        assert(g_gpio_levels[ch] == level);
    }
    printf("  [PASS] Write to all 8 channels\n");

    /* Read from all channels */
    for (int ch = 0; ch < 8; ch++) {
        int32_t level = xy_mux_gpio_read(&mgr, ch);
        xy_mux_gpio_level_t expected = (ch % 2) ? XY_MUX_GPIO_HIGH : XY_MUX_GPIO_LOW;
        assert(level == expected);
    }
    printf("  [PASS] Read from all 8 channels\n");

    /* Toggle all channels */
    for (int ch = 0; ch < 8; ch++) {
        xy_mux_gpio_toggle(&mgr, ch);
        xy_mux_gpio_level_t expected = (ch % 2) ? XY_MUX_GPIO_LOW : XY_MUX_GPIO_HIGH;
        assert(g_gpio_levels[ch] == expected);
    }
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
    assert(ret == XY_MUX_OK);
    assert(packet_len == 5); /* 4 byte header + 1 byte data */
    printf("  [PASS] GPIO packet built: %d bytes\n", (int)packet_len);

    /* Verify header structure */
    assert(tx_buffer[0] == XY_MUX_TYPE_GPIO);
    assert(tx_buffer[1] == 0);
    assert(tx_buffer[2] == 1); /* length */
    assert(tx_buffer[3] == 0);
    printf("  [PASS] TLV header structure verified\n");

    /* Process the packet */
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    assert(ret == XY_MUX_OK);
    assert(g_gpio_levels[0] == XY_MUX_GPIO_HIGH);
    printf("  [PASS] Packet processed, GPIO set to HIGH\n");

    xy_mux_deinit(&mgr);
}

/* ==================== Main Entry ==================== */

int main(void)
{
    printf("========================================\n");
    printf("XY_MUX GPIO Test Suite\n");
    printf("========================================\n");

    /* Run all tests */
    test_gpio_register();
    test_gpio_write();
    test_gpio_read();
    test_gpio_toggle();
    test_gpio_config();
    test_gpio_error_handling();
    test_gpio_multi_channel();
    test_gpio_tlv_packet();

    printf("\n========================================\n");
    printf("All GPIO Tests PASSED!\n");
    printf("========================================\n");

    return 0;
}
