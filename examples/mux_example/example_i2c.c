/**
 * @file example_i2c.c
 * @brief I2C Bus Multiplexing Example
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Demonstrates I2C bus multiplexing using the XY_MUX framework.
 * Shows how to register I2C buses and perform read/write operations.
 */

#include "xy_mux.h"
#include "xy_mux_i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example buffer size */
#define BUFFER_SIZE 512

/* ==================== Custom I2C Operations ==================== */

/**
 * @brief Custom I2C write implementation
 */
static int32_t custom_i2c_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    printf("[I2C-%d] Write %d bytes: ", channel, (int)len);
    const uint8_t *buf = (const uint8_t *)data;
    for (size_t i = 0; i < len && i < 16; i++) {
        printf("%02X ", buf[i]);
    }
    if (len > 16) printf("...");
    printf("\n");
    return len;
}

/**
 * @brief Custom I2C read implementation
 */
static int32_t custom_i2c_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* Simulate reading I2C data - fill with pattern */
    uint8_t *buf = (uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(0xA5 + i);
    }
    printf("[I2C-%d] Read %d bytes (simulated)\n", channel, (int)len);
    return len;
}

/**
 * @brief Custom I2C ioctl implementation
 */
static int32_t custom_i2c_ioctl(uint8_t channel, int cmd, void *arg)
{
    printf("[I2C-%d] IOCTL: cmd=%d\n", channel, cmd);

    switch (cmd) {
        case XY_MUX_I2C_CMD_SET_SPEED:
            if (arg) {
                uint32_t speed = *(uint32_t *)arg;
                printf("[I2C-%d] Set speed: %lu Hz\n", channel, (unsigned long)speed);
            }
            break;
        case XY_MUX_I2C_CMD_GET_SPEED:
            if (arg) {
                *(uint32_t *)arg = 400000; /* 400kHz */
            }
            break;
        case XY_MUX_I2C_CMD_SCAN:
            printf("[I2C-%d] Scan requested\n", channel);
            break;
        default:
            break;
    }

    return XY_MUX_OK;
}

/* ==================== Example Functions ==================== */

/**
 * @brief Example: Basic I2C operations
 */
static void example_basic_i2c(void)
{
    printf("\n=== Basic I2C Example ===\n");

    /* Allocate buffers */
    uint8_t *tx_buffer = (uint8_t *)malloc(BUFFER_SIZE);
    uint8_t *rx_buffer = (uint8_t *)malloc(BUFFER_SIZE);
    if (!tx_buffer || !rx_buffer) {
        printf("Failed to allocate buffers\n");
        return;
    }

    /* Initialize MUX manager */
    xy_mux_manager_t mgr;
    int32_t ret = xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);
    if (ret != XY_MUX_OK) {
        printf("MUX init failed: %d\n", ret);
        free(tx_buffer);
        free(rx_buffer);
        return;
    }

    /* Create custom I2C ops */
    xy_mux_ops_t i2c_ops = {
        .init = NULL,
        .deinit = NULL,
        .read = custom_i2c_read,
        .write = custom_i2c_write,
        .ioctl = custom_i2c_ioctl,
    };

    /* Register I2C channels */
    ret = xy_mux_i2c_register(&mgr, 0, &i2c_ops, NULL);
    if (ret != XY_MUX_OK) {
        printf("Failed to register I2C-0: %d\n", ret);
    }

    /* Configure I2C bus */
    xy_mux_i2c_config_t config = {
        .speed = 400000,
        .addr_bits = 7,
    };
    xy_mux_i2c_config(&mgr, 0, &config);

    /* Perform I2C operations */
    printf("\n--- I2C Write Operations ---\n");
    uint8_t write_data[] = {0x55, 0xAA, 0x01, 0x02};
    xy_mux_i2c_write(&mgr, 0, 0x50, write_data, sizeof(write_data));

    printf("\n--- I2C Read Operations ---\n");
    uint8_t read_buffer[16];
    ret = xy_mux_i2c_read(&mgr, 0, 0x50, read_buffer, sizeof(read_buffer));
    printf("Read returned: %d\n", ret);

    /* Print device count */
    printf("\nTotal devices registered: %d\n", xy_mux_get_device_count(&mgr));

    /* Cleanup */
    xy_mux_deinit(&mgr);
    free(tx_buffer);
    free(rx_buffer);

    printf("\n=== Basic I2C Example Complete ===\n");
}

/**
 * @brief Example: I2C transfer with messages
 */
static void example_i2c_transfer(void)
{
    printf("\n=== I2C Transfer Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t i2c_ops = {
        .read = custom_i2c_read,
        .write = custom_i2c_write,
        .ioctl = custom_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &i2c_ops, NULL);

    /* Perform I2C transfer with multiple messages */
    uint8_t reg_addr = 0x10;
    uint8_t data_out[] = {0x11, 0x22, 0x33};
    uint8_t data_in[8];

    xy_mux_i2c_msg_t msgs[] = {
        {0x50, 0, 1, &reg_addr},      /* Write register address */
        {0x50, 0, 3, data_out},       /* Write data */
        {0x50, XY_MUX_I2C_M_RD, 8, data_in}, /* Read response */
    };

    printf("\n--- I2C Multi-message Transfer ---\n");
    int32_t ret = xy_mux_i2c_transfer(&mgr, 0, msgs, 3);
    printf("Transfer returned: %d\n", ret);

    xy_mux_deinit(&mgr);

    printf("\n=== I2C Transfer Example Complete ===\n");
}

/**
 * @brief Example: I2C device scanning
 */
static void example_i2c_scan(void)
{
    printf("\n=== I2C Scan Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t i2c_ops = {
        .read = custom_i2c_read,
        .write = custom_i2c_write,
        .ioctl = custom_i2c_ioctl,
    };
    xy_mux_i2c_register(&mgr, 0, &i2c_ops, NULL);

    /* Scan for I2C devices (simulated) */
    uint16_t addrs[16];
    int32_t count = xy_mux_i2c_scan(&mgr, 0, addrs, 16);

    printf("\n--- I2C Device Scan Results ---\n");
    if (count > 0) {
        printf("Found %d I2C devices:\n", count);
        for (int i = 0; i < count; i++) {
            printf("  0x%02X\n", addrs[i]);
        }
    } else {
        printf("No I2C devices found (scan not supported in demo)\n");
    }

    xy_mux_deinit(&mgr);

    printf("\n=== I2C Scan Example Complete ===\n");
}

/**
 * @brief Example: Multiple I2C buses
 */
static void example_multi_i2c(void)
{
    printf("\n=== Multiple I2C Buses Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Register multiple I2C buses with different speeds */
    xy_mux_ops_t i2c_ops = {
        .read = custom_i2c_read,
        .write = custom_i2c_write,
        .ioctl = custom_i2c_ioctl,
    };

    /* Bus 0: High speed (400kHz) */
    xy_mux_i2c_register(&mgr, 0, &i2c_ops, NULL);
    xy_mux_i2c_config_t config0 = {.speed = 400000, .addr_bits = 7};
    xy_mux_i2c_config(&mgr, 0, &config0);

    /* Bus 1: Standard speed (100kHz) */
    xy_mux_i2c_register(&mgr, 1, &i2c_ops, NULL);
    xy_mux_i2c_config_t config1 = {.speed = 100000, .addr_bits = 7};
    xy_mux_i2c_config(&mgr, 1, &config1);

    /* Bus 2: Fast mode (1MHz) */
    xy_mux_i2c_register(&mgr, 2, &i2c_ops, NULL);
    xy_mux_i2c_config_t config2 = {.speed = 1000000, .addr_bits = 10};
    xy_mux_i2c_config(&mgr, 2, &config2);

    printf("\n--- Writing to multiple I2C buses ---\n");
    uint8_t data = 0xAB;
    xy_mux_i2c_write(&mgr, 0, 0x50, &data, 1);
    xy_mux_i2c_write(&mgr, 1, 0x30, &data, 1);
    xy_mux_i2c_write(&mgr, 2, 0x40, &data, 1);

    printf("Total devices: %d\n", xy_mux_get_device_count(&mgr));

    xy_mux_deinit(&mgr);

    printf("\n=== Multiple I2C Buses Example Complete ===\n");
}

/* ==================== Main Entry ==================== */

int main(int argc, char *argv[])
{
    printf("XY_MUX I2C Bus Multiplexing Example\n");
    printf("Version: %s\n", XY_MUX_VERSION_STRING);
    printf("====================================\n");

    /* Run examples */
    example_basic_i2c();
    example_i2c_transfer();
    example_i2c_scan();
    example_multi_i2c();

    printf("\nAll I2C examples completed successfully!\n");
    return 0;
}
