/**
 * @file example_spi.c
 * @brief SPI Device Multiplexing Example
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Demonstrates SPI device multiplexing using the XY_MUX framework.
 * Shows how to register SPI buses and perform data transfers.
 */

#include "xy_mux.h"
#include "xy_mux_spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example buffer size */
#define BUFFER_SIZE 512

/* ==================== Custom SPI Operations ==================== */

/**
 * @brief Custom SPI write implementation
 */
static int32_t custom_spi_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    printf("[SPI-%d] Write %d bytes: ", channel, (int)len);
    const uint8_t *buf = (const uint8_t *)data;
    for (size_t i = 0; i < len && i < 16; i++) {
        printf("%02X ", buf[i]);
    }
    if (len > 16) printf("...");
    printf("\n");
    return len;
}

/**
 * @brief Custom SPI read implementation
 */
static int32_t custom_spi_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* Simulate reading SPI data - fill with pattern */
    uint8_t *buf = (uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(0x55 + i);
    }
    printf("[SPI-%d] Read %d bytes (simulated)\n", channel, (int)len);
    return len;
}

/**
 * @brief Custom SPI ioctl implementation
 */
static int32_t custom_spi_ioctl(uint8_t channel, int cmd, void *arg)
{
    printf("[SPI-%d] IOCTL: cmd=%d\n", channel, cmd);

    switch (cmd) {
        case XY_MUX_SPI_CMD_SET_SPEED:
            if (arg) {
                uint32_t speed = *(uint32_t *)arg;
                printf("[SPI-%d] Set speed: %lu Hz\n", channel, (unsigned long)speed);
            }
            break;
        case XY_MUX_SPI_CMD_GET_SPEED:
            if (arg) {
                *(uint32_t *)arg = 10000000; /* 10MHz */
            }
            break;
        case XY_MUX_SPI_CMD_SET_MODE:
            if (arg) {
                xy_mux_spi_mode_t mode = *(xy_mux_spi_mode_t *)arg;
                printf("[SPI-%d] Set mode: %d\n", channel, mode);
            }
            break;
        default:
            break;
    }

    return XY_MUX_OK;
}

/* ==================== Example Functions ==================== */

/**
 * @brief Example: Basic SPI operations
 */
static void example_basic_spi(void)
{
    printf("\n=== Basic SPI Example ===\n");

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

    /* Create custom SPI ops */
    xy_mux_ops_t spi_ops = {
        .init = NULL,
        .deinit = NULL,
        .read = custom_spi_read,
        .write = custom_spi_write,
        .ioctl = custom_spi_ioctl,
    };

    /* Register SPI channel */
    ret = xy_mux_spi_register(&mgr, 0, &spi_ops, NULL);
    if (ret != XY_MUX_OK) {
        printf("Failed to register SPI-0: %d\n", ret);
    }

    /* Configure SPI bus */
    xy_mux_spi_config_t config = {
        .speed = 10000000,
        .mode = XY_MUX_SPI_MODE0,
        .bits = 8,
        .cs_pin = 0,
    };
    xy_mux_spi_config(&mgr, 0, &config);

    /* Perform SPI operations */
    printf("\n--- SPI Write Operations ---\n");
    uint8_t write_data[] = {0x01, 0x02, 0x03, 0x04};
    xy_mux_spi_write(&mgr, 0, write_data, sizeof(write_data));

    printf("\n--- SPI Read Operations ---\n");
    uint8_t read_buffer[16];
    ret = xy_mux_spi_read(&mgr, 0, read_buffer, sizeof(read_buffer));
    printf("Read returned: %d\n", ret);

    /* Print device count */
    printf("\nTotal devices registered: %d\n", xy_mux_get_device_count(&mgr));

    /* Cleanup */
    xy_mux_deinit(&mgr);
    free(tx_buffer);
    free(rx_buffer);

    printf("\n=== Basic SPI Example Complete ===\n");
}

/**
 * @brief Example: Full-duplex SPI transfer
 */
static void example_spi_transfer(void)
{
    printf("\n=== SPI Full-Duplex Transfer Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t spi_ops = {
        .read = custom_spi_read,
        .write = custom_spi_write,
        .ioctl = custom_spi_ioctl,
    };
    xy_mux_spi_register(&mgr, 0, &spi_ops, NULL);

    /* Perform full-duplex transfer */
    uint8_t tx_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t rx_data[4];

    printf("\n--- SPI Full-Duplex Transfer ---\n");
    printf("TX: %02X %02X %02X %02X\n", tx_data[0], tx_data[1], tx_data[2], tx_data[3]);

    int32_t ret = xy_mux_spi_transfer(&mgr, 0, tx_data, rx_data, sizeof(tx_data));
    printf("Transfer returned: %d\n", ret);

    if (ret > 0) {
        printf("RX: %02X %02X %02X %02X\n", rx_data[0], rx_data[1], rx_data[2], rx_data[3]);
    }

    xy_mux_deinit(&mgr);

    printf("\n=== SPI Full-Duplex Transfer Example Complete ===\n");
}

/**
 * @brief Example: Multiple SPI devices
 */
static void example_multi_spi_devices(void)
{
    printf("\n=== Multiple SPI Devices Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t spi_ops = {
        .read = custom_spi_read,
        .write = custom_spi_write,
        .ioctl = custom_spi_ioctl,
    };

    /* Register multiple SPI devices on different channels */
    xy_mux_spi_register(&mgr, 0, &spi_ops, NULL); /* Flash memory */
    xy_mux_spi_config_t config0 = {.speed = 20000000, .mode = XY_MUX_SPI_MODE0, .bits = 8, .cs_pin = 0};
    xy_mux_spi_config(&mgr, 0, &config0);

    xy_mux_spi_register(&mgr, 1, &spi_ops, NULL); /* Sensor */
    xy_mux_spi_config_t config1 = {.speed = 1000000, .mode = XY_MUX_SPI_MODE3, .bits = 8, .cs_pin = 1};
    xy_mux_spi_config(&mgr, 1, &config1);

    xy_mux_spi_register(&mgr, 2, &spi_ops, NULL); /* Display */
    xy_mux_spi_config_t config2 = {.speed = 8000000, .mode = XY_MUX_SPI_MODE1, .bits = 16, .cs_pin = 2};
    xy_mux_spi_config(&mgr, 2, &config2);

    printf("\n--- Accessing Multiple SPI Devices ---\n");

    /* Write to Flash */
    uint8_t flash_cmd[] = {0x06}; /* Write enable */
    xy_mux_spi_write(&mgr, 0, flash_cmd, sizeof(flash_cmd));

    /* Write to Sensor */
    uint8_t sensor_cmd[] = {0x80, 0x00};
    xy_mux_spi_write(&mgr, 1, sensor_cmd, sizeof(sensor_cmd));

    /* Transfer with Display */
    uint8_t display_tx[] = {0x01, 0x02};
    uint8_t display_rx[2];
    xy_mux_spi_transfer(&mgr, 2, display_tx, display_rx, sizeof(display_tx));

    printf("Total devices: %d\n", xy_mux_get_device_count(&mgr));

    xy_mux_deinit(&mgr);

    printf("\n=== Multiple SPI Devices Example Complete ===\n");
}

/**
 * @brief Example: SPI packet building
 */
static void example_spi_tlv_packet(void)
{
    printf("\n=== SPI TLV Packet Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Build SPI packet manually */
    uint8_t spi_data[] = {0xAA, 0xBB, 0xCC};
    size_t packet_len = 0;

    int32_t ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_SPI, 0,
                                       spi_data, sizeof(spi_data),
                                       tx_buffer, &packet_len);
    if (ret == XY_MUX_OK) {
        printf("Built SPI packet: %d bytes\n", (int)packet_len);
        printf("  Type: %d (SPI)\n", tx_buffer[0]);
        printf("  Channel: %d\n", tx_buffer[1]);
        printf("  Length: %d\n", tx_buffer[2] | (tx_buffer[3] << 8));
        printf("  Data: ");
        for (size_t i = 4; i < packet_len; i++) {
            printf("%02X ", tx_buffer[i]);
        }
        printf("\n");
    }

    xy_mux_deinit(&mgr);

    printf("\n=== SPI TLV Packet Example Complete ===\n");
}

/* ==================== Main Entry ==================== */

int main(int argc, char *argv[])
{
    printf("XY_MUX SPI Device Multiplexing Example\n");
    printf("Version: %s\n", XY_MUX_VERSION_STRING);
    printf("======================================\n");

    /* Run examples */
    example_basic_spi();
    example_spi_transfer();
    example_multi_spi_devices();
    example_spi_tlv_packet();

    printf("\nAll SPI examples completed successfully!\n");
    return 0;
}
