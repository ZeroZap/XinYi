/**
 * @file example_uart.c
 * @brief UART Port Sharing Example
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Demonstrates UART port sharing/multiplexing using the XY_MUX framework.
 * Shows how to register multiple virtual UART ports over a single physical link.
 */

#include "xy_mux.h"
#include "xy_mux_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example buffer size */
#define BUFFER_SIZE 512

/* ==================== Custom UART Operations ==================== */

/**
 * @brief Custom UART write implementation
 */
static int32_t custom_uart_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    printf("[UART-%d] Write %d bytes: ", channel, (int)len);
    const uint8_t *buf = (const uint8_t *)data;
    for (size_t i = 0; i < len && i < 32; i++) {
        if (buf[i] >= 32 && buf[i] < 127) {
            printf("%c", buf[i]);
        } else {
            printf("[%02X]", buf[i]);
        }
    }
    if (len > 32) printf("...");
    printf("\n");
    return len;
}

/**
 * @brief Custom UART read implementation
 */
static int32_t custom_uart_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* Simulate reading UART data */
    uint8_t *buf = (uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)('A' + (i % 26));
    }
    printf("[UART-%d] Read %d bytes (simulated)\n", channel, (int)len);
    return len;
}

/**
 * @brief Custom UART ioctl implementation
 */
static int32_t custom_uart_ioctl(uint8_t channel, int cmd, void *arg)
{
    printf("[UART-%d] IOCTL: cmd=%d\n", channel, cmd);

    switch (cmd) {
        case XY_MUX_UART_CMD_SET_BAUD:
            if (arg) {
                uint32_t baud = *(uint32_t *)arg;
                printf("[UART-%d] Set baudrate: %lu\n", channel, (unsigned long)baud);
            }
            break;
        case XY_MUX_UART_CMD_GET_BAUD:
            if (arg) {
                *(uint32_t *)arg = 115200;
            }
            break;
        case XY_MUX_UART_CMD_FLUSH:
            printf("[UART-%d] Flush\n", channel);
            break;
        default:
            break;
    }

    return XY_MUX_OK;
}

/* ==================== Example Functions ==================== */

/**
 * @brief Example: Basic UART operations
 */
static void example_basic_uart(void)
{
    printf("\n=== Basic UART Example ===\n");

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

    /* Create custom UART ops */
    xy_mux_ops_t uart_ops = {
        .init = NULL,
        .deinit = NULL,
        .read = custom_uart_read,
        .write = custom_uart_write,
        .ioctl = custom_uart_ioctl,
    };

    /* Register UART channels */
    ret = xy_mux_uart_register(&mgr, 0, &uart_ops, NULL);
    if (ret != XY_MUX_OK) {
        printf("Failed to register UART-0: %d\n", ret);
    }

    /* Configure UART port */
    xy_mux_uart_config_t config = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
        .flow_control = 0,
    };
    xy_mux_uart_config(&mgr, 0, &config);

    /* Perform UART operations */
    printf("\n--- UART Write Operations ---\n");
    const char *msg = "Hello UART!";
    xy_mux_uart_write(&mgr, 0, msg, strlen(msg), 100);

    printf("\n--- UART Read Operations ---\n");
    uint8_t read_buffer[32];
    ret = xy_mux_uart_read(&mgr, 0, read_buffer, sizeof(read_buffer), 100);
    printf("Read returned: %d\n", ret);

    /* Print device count */
    printf("\nTotal devices registered: %d\n", xy_mux_get_device_count(&mgr));

    /* Cleanup */
    xy_mux_deinit(&mgr);
    free(tx_buffer);
    free(rx_buffer);

    printf("\n=== Basic UART Example Complete ===\n");
}

/**
 * @brief Example: Multiple virtual UART ports
 */
static void example_multi_uart(void)
{
    printf("\n=== Multiple Virtual UART Ports Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t uart_ops = {
        .read = custom_uart_read,
        .write = custom_uart_write,
        .ioctl = custom_uart_ioctl,
    };

    /* Register multiple virtual UART ports */
    /* Port 0: Debug console (115200 baud) */
    xy_mux_uart_register(&mgr, 0, &uart_ops, NULL);
    xy_mux_uart_config_t cfg0 = {.baudrate = 115200, .data_bits = 8, .stop_bits = 1, .parity = 0, .flow_control = 0};
    xy_mux_uart_config(&mgr, 0, &cfg0);

    /* Port 1: GPS module (9600 baud) */
    xy_mux_uart_register(&mgr, 1, &uart_ops, NULL);
    xy_mux_uart_config_t cfg1 = {.baudrate = 9600, .data_bits = 8, .stop_bits = 1, .parity = 0, .flow_control = 0};
    xy_mux_uart_config(&mgr, 1, &cfg1);

    /* Port 2: Modem (57600 baud) */
    xy_mux_uart_register(&mgr, 2, &uart_ops, NULL);
    xy_mux_uart_config_t cfg2 = {.baudrate = 57600, .data_bits = 8, .stop_bits = 1, .parity = 0, .flow_control = 0};
    xy_mux_uart_config(&mgr, 2, &cfg2);

    printf("\n--- Sending data on multiple UART ports ---\n");

    /* Send to debug console */
    const char *debug_msg = "System ready\r\n";
    xy_mux_uart_write(&mgr, 0, debug_msg, strlen(debug_msg), 50);

    /* Send to GPS */
    const char *gps_msg = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,47.0,M,,*47\r\n";
    xy_mux_uart_write(&mgr, 1, gps_msg, strlen(gps_msg), 100);

    /* Send to modem */
    const char *modem_msg = "AT+CSQ\r\n";
    xy_mux_uart_write(&mgr, 2, modem_msg, strlen(modem_msg), 100);

    printf("Total devices: %d\n", xy_mux_get_device_count(&mgr));

    xy_mux_deinit(&mgr);

    printf("\n=== Multiple Virtual UART Ports Example Complete ===\n");
}

/**
 * @brief Example: UART TLV packet processing
 */
static void example_uart_tlv_packet(void)
{
    printf("\n=== UART TLV Packet Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Build UART data packet */
    const char *uart_data = "AT\r\n";
    size_t packet_len = 0;

    int32_t ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_UART, 0,
                                       uart_data, strlen(uart_data),
                                       tx_buffer, &packet_len);
    if (ret == XY_MUX_OK) {
        printf("Built UART packet: %d bytes\n", (int)packet_len);
        printf("  Type: %d (UART)\n", tx_buffer[0]);
        printf("  Channel: %d\n", tx_buffer[1]);
        printf("  Length: %d\n", tx_buffer[2] | (tx_buffer[3] << 8));
        printf("  Data: ");
        for (size_t i = 4; i < packet_len; i++) {
            if (tx_buffer[i] >= 32 && tx_buffer[i] < 127) {
                printf("%c", tx_buffer[i]);
            } else {
                printf("[%02X]", tx_buffer[i]);
            }
        }
        printf("\n");
    }

    /* Process the packet */
    printf("\n--- Processing packet ---\n");
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    printf("Process result: %d\n", ret);

    xy_mux_deinit(&mgr);

    printf("\n=== UART TLV Packet Example Complete ===\n");
}

/**
 * @brief Example: UART port sharing scenario
 */
static void example_uart_sharing_scenario(void)
{
    printf("\n=== UART Port Sharing Scenario ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t uart_ops = {
        .read = custom_uart_read,
        .write = custom_uart_write,
        .ioctl = custom_uart_ioctl,
    };

    /* Single physical UART, multiple virtual channels */
    xy_mux_uart_register(&mgr, 0, &uart_ops, NULL); /* Console */
    xy_mux_uart_register(&mgr, 1, &uart_ops, NULL); /* AT commands */
    xy_mux_uart_register(&mgr, 2, &uart_ops, NULL); /* Debug log */

    printf("Virtual UART ports: Console, AT, Debug\n");
    printf("Physical UART: Single shared link\n\n");

    /* Virtual port 0: Console output */
    printf("--- Console Output ---\n");
    const char *console = "XinYi> ";
    xy_mux_uart_write(&mgr, 0, console, strlen(console), 10);

    /* Virtual port 1: AT command response */
    printf("\n--- AT Command Response ---\n");
    const char *at_resp = "OK\r\n";
    xy_mux_uart_write(&mgr, 1, at_resp, strlen(at_resp), 10);

    /* Virtual port 2: Debug log */
    printf("\n--- Debug Log ---\n");
    const char *debug = "[DBG] UART initialized\r\n";
    xy_mux_uart_write(&mgr, 2, debug, strlen(debug), 10);

    printf("\nTotal devices: %d (all sharing single physical UART)\n",
           xy_mux_get_device_count(&mgr));

    xy_mux_deinit(&mgr);

    printf("\n=== UART Port Sharing Scenario Complete ===\n");
}

/* ==================== Main Entry ==================== */

int main(int argc, char *argv[])
{
    printf("XY_MUX UART Port Sharing Example\n");
    printf("Version: %s\n", XY_MUX_VERSION_STRING);
    printf("==================================\n");

    /* Run examples */
    example_basic_uart();
    example_multi_uart();
    example_uart_tlv_packet();
    example_uart_sharing_scenario();

    printf("\nAll UART examples completed successfully!\n");
    return 0;
}
