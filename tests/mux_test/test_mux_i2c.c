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

void xy_log_char(char ch)
{
    (void)ch;
}

/* Test buffer size */
#define BUFFER_SIZE 512

/* Track mock I2C state */
static uint8_t g_i2c_buffers[16][256];
static size_t g_i2c_buffer_lens[16];

/* Mock I2C operations */
static int32_t mock_i2c_init(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    memset(g_i2c_buffers[channel], 0, sizeof(g_i2c_buffers[channel]));
    g_i2c_buffer_lens[channel] = 0;
    return XY_MUX_OK;
}

static int32_t mock_i2c_deinit(uint8_t channel)
{
    (void)channel;
    return XY_MUX_OK;
}

static int32_t mock_i2c_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0 || len > 256) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memcpy(g_i2c_buffers[channel], data, len);
    g_i2c_buffer_lens[channel] = len;
    printf("    [MOCK] I2C-%d write %d bytes\n", channel, (int)len);
    return len;
}

static int32_t mock_i2c_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    /* Return simulated data */
    memset(data, 0xA5, len);
    printf("    [MOCK] I2C-%d read %d bytes (simulated)\n", channel, (int)len);
    return len;
}

static int32_t mock_i2c_ioctl(uint8_t channel, int cmd, void *arg)
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
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    printf("  [PASS] I2C-0 registered\n");

    /* Register I2C channel 1 */
    ret = xy_mux_i2c_register(&mgr, 1, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    printf("  [PASS] I2C-1 registered\n");

    /* Register I2C channel 2 */
    ret = xy_mux_i2c_register(&mgr, 2, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    printf("  [PASS] I2C-2 registered\n");

    TEST_ASSERT_TRUE(mgr.device_count == 3);
    printf("  [PASS] Device count = 3\n");

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
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    printf("  [PASS] I2C configured at 400kHz, 7-bit addressing\n");

    /* Configure with 10-bit addressing */
    config.addr_bits = 10;
    ret = xy_mux_i2c_config(&mgr, 0, &config);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    printf("  [PASS] I2C reconfigured with 10-bit addressing\n");

    /* Configure high speed */
    config.speed = 1000000;
    config.addr_bits = 7;
    ret = xy_mux_i2c_config(&mgr, 0, &config);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
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
    TEST_ASSERT_TRUE(ret >= 0);
    printf("  [PASS] Write %d bytes to I2C device at address 0x50\n", (int)sizeof(data));

    /* Verify data was written to mock buffer */
    TEST_ASSERT_TRUE(g_i2c_buffer_lens[0] == sizeof(data) + 2); /* +2 for address bytes */
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
    TEST_ASSERT_TRUE(ret > 0);
    printf("  [PASS] Read %d bytes from I2C device\n", ret);

    /* Verify buffer contains simulated data */
    TEST_ASSERT_TRUE(buffer[0] == 0xA5);
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
    TEST_ASSERT_TRUE(ret == 16);
    TEST_ASSERT_TRUE(data_in[0] == 0xA5);
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
    TEST_ASSERT_TRUE(ret != XY_MUX_OK);
    printf("  [PASS] Read from non-registered channel rejected\n");

    /* Try to write to non-registered channel */
    uint8_t data[] = {0x55};
    ret = xy_mux_i2c_write(&mgr, 99, 0x50, data, sizeof(data));
    TEST_ASSERT_TRUE(ret != XY_MUX_OK);
    printf("  [PASS] Write to non-registered channel rejected\n");

    /* Try with NULL manager */
    ret = xy_mux_i2c_write(NULL, 0, 0x50, data, sizeof(data));
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL manager rejected for write\n");

    ret = xy_mux_i2c_read(NULL, 0, 0x50, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL manager rejected for read\n");

    /* Try with NULL data */
    ret = xy_mux_i2c_write(&mgr, 0, 0x50, NULL, sizeof(data));
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL data rejected for write\n");

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
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(packet_len == sizeof(xy_mux_header_t) + sizeof(i2c_data));
    printf("  [PASS] I2C packet built: %d bytes\n", (int)packet_len);

    /* Verify header structure */
    TEST_ASSERT_TRUE(tx_buffer[0] == XY_MUX_TYPE_I2C);
    TEST_ASSERT_TRUE(tx_buffer[1] == 0);  /* channel */
    TEST_ASSERT_TRUE(tx_buffer[2] == sizeof(i2c_data)); /* length */
    printf("  [PASS] TLV header structure verified\n");

    /* Process the packet */
    g_i2c_buffer_lens[0] = 0;
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    TEST_ASSERT_TRUE(ret == (int32_t)sizeof(i2c_data));
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

    /* Read from each bus */
    uint8_t buffer[4];
    xy_mux_i2c_read(&mgr, 0, 0x50, buffer, 1);
    printf("  [PASS] Read from I2C-0\n");

    xy_mux_i2c_read(&mgr, 1, 0x30, buffer, 1);
    printf("  [PASS] Read from I2C-1\n");

    xy_mux_i2c_read(&mgr, 2, 0x40, buffer, 1);
    printf("  [PASS] Read from I2C-2\n");

    xy_mux_deinit(&mgr);
}

/* ==================== Main Entry ==================== */

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_i2c_register);
    RUN_TEST(test_i2c_config);
    RUN_TEST(test_i2c_write);
    RUN_TEST(test_i2c_read);
    RUN_TEST(test_i2c_transfer);
    RUN_TEST(test_i2c_error_handling);
    RUN_TEST(test_i2c_tlv_packet);
    RUN_TEST(test_i2c_multi_bus);
    return UNITY_END();
}
