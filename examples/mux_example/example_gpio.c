/**
 * @file example_gpio.c
 * @brief GPIO Multiplexing Example
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Demonstrates GPIO multiplexing using the XY_MUX framework.
 * Shows how to register GPIO devices and perform read/write operations.
 */

#include "xy_mux.h"
#include "xy_mux_gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example buffer size */
#define BUFFER_SIZE 512

/* ==================== Custom GPIO Operations ==================== */

/**
 * @brief Custom GPIO write implementation
 */
static int32_t custom_gpio_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    uint8_t level = ((const uint8_t *)data)[0];
    printf("[GPIO-%d] Write: %s\n", channel, level ? "HIGH" : "LOW");
    return sizeof(uint8_t);
}

/**
 * @brief Custom GPIO read implementation
 */
static int32_t custom_gpio_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* Simulate reading GPIO state - always return HIGH for demo */
    uint8_t level = XY_MUX_GPIO_HIGH;
    memcpy(data, &level, sizeof(uint8_t));
    printf("[GPIO-%d] Read: HIGH (simulated)\n", channel);
    return sizeof(uint8_t);
}

/**
 * @brief Custom GPIO ioctl implementation
 */
static int32_t custom_gpio_ioctl(uint8_t channel, int cmd, void *arg)
{
    printf("[GPIO-%d] IOCTL: cmd=%d\n", channel, cmd);

    switch (cmd) {
        case XY_MUX_GPIO_CMD_SET_LEVEL:
            if (arg) {
                uint8_t level = *(uint8_t *)arg;
                printf("[GPIO-%d] Set level: %s\n", channel, level ? "HIGH" : "LOW");
            }
            break;
        case XY_MUX_GPIO_CMD_GET_LEVEL:
            if (arg) {
                *(uint8_t *)arg = XY_MUX_GPIO_HIGH;
            }
            break;
        case XY_MUX_GPIO_CMD_TOGGLE:
            printf("[GPIO-%d] Toggle\n", channel);
            break;
        default:
            break;
    }

    return XY_MUX_OK;
}

/* ==================== Example Functions ==================== */

/**
 * @brief Example: Basic GPIO operations
 */
static void example_basic_gpio(void)
{
    printf("\n=== Basic GPIO Example ===\n");

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

    /* Create custom GPIO ops */
    xy_mux_ops_t gpio_ops = {
        .init = NULL,
        .deinit = NULL,
        .read = custom_gpio_read,
        .write = custom_gpio_write,
        .ioctl = custom_gpio_ioctl,
    };

    /* Register GPIO channels */
    ret = xy_mux_gpio_register(&mgr, 0, &gpio_ops, NULL);
    if (ret != XY_MUX_OK) {
        printf("Failed to register GPIO-0: %d\n", ret);
    }

    ret = xy_mux_gpio_register(&mgr, 1, &gpio_ops, NULL);
    if (ret != XY_MUX_OK) {
        printf("Failed to register GPIO-1: %d\n", ret);
    }

    /* Perform GPIO operations */
    printf("\n--- GPIO Write Operations ---\n");
    xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_HIGH);
    xy_mux_gpio_write(&mgr, 0, XY_MUX_GPIO_LOW);
    xy_mux_gpio_write(&mgr, 1, XY_MUX_GPIO_HIGH);

    printf("\n--- GPIO Read Operations ---\n");
    int32_t level = xy_mux_gpio_read(&mgr, 0);
    printf("GPIO-0 level: %d\n", level);

    level = xy_mux_gpio_read(&mgr, 1);
    printf("GPIO-1 level: %d\n", level);

    printf("\n--- GPIO Toggle Operations ---\n");
    xy_mux_gpio_toggle(&mgr, 0);
    xy_mux_gpio_toggle(&mgr, 1);

    /* Print device count */
    printf("\nTotal devices registered: %d\n", xy_mux_get_device_count(&mgr));

    /* Cleanup */
    xy_mux_deinit(&mgr);
    free(tx_buffer);
    free(rx_buffer);

    printf("\n=== Basic GPIO Example Complete ===\n");
}

/**
 * @brief Example: TLV packet building for GPIO
 */
static void example_gpio_tlv_packet(void)
{
    printf("\n=== GPIO TLV Packet Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Build GPIO write packet */
    uint8_t gpio_data = XY_MUX_GPIO_HIGH;
    size_t packet_len = 0;

    int32_t ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_GPIO, 0,
                                       &gpio_data, sizeof(gpio_data),
                                       tx_buffer, &packet_len);
    if (ret == XY_MUX_OK) {
        printf("Built GPIO packet: %d bytes\n", packet_len);
        printf("  Type: %d (GPIO)\n", tx_buffer[0]);
        printf("  Channel: %d\n", tx_buffer[1]);
        printf("  Length: %d\n", tx_buffer[2] | (tx_buffer[3] << 8));
    }

    /* Process the packet */
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    printf("Packet processed: %d\n", ret);

    xy_mux_deinit(&mgr);

    printf("\n=== GPIO TLV Packet Example Complete ===\n");
}

/**
 * @brief Example: Multiple GPIO channels
 */
static void example_multi_gpio(void)
{
    printf("\n=== Multiple GPIO Channels Example ===\n");

    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    xy_mux_manager_t mgr;
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Register 8 GPIO channels (0-7) */
    for (int ch = 0; ch < 8; ch++) {
        xy_mux_ops_t ops = {
            .read = custom_gpio_read,
            .write = custom_gpio_write,
            .ioctl = custom_gpio_ioctl,
        };

        int32_t ret = xy_mux_gpio_register(&mgr, ch, &ops, NULL);
        printf("Register GPIO-%d: %s\n", ch, ret == XY_MUX_OK ? "OK" : "FAIL");
    }

    /* Toggle all GPIOs */
    printf("\n--- Toggling all GPIO channels ---\n");
    for (int ch = 0; ch < 8; ch++) {
        xy_mux_gpio_toggle(&mgr, ch);
    }

    /* Read all GPIOs */
    printf("\n--- Reading all GPIO channels ---\n");
    for (int ch = 0; ch < 8; ch++) {
        int32_t level = xy_mux_gpio_read(&mgr, ch);
        printf("GPIO-%d: %s\n", ch, level ? "HIGH" : "LOW");
    }

    /* Unregister some channels */
    printf("\n--- Unregistering GPIO channels 2, 4, 6 ---\n");
    xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 2);
    xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 4);
    xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 6);

    printf("Remaining devices: %d\n", xy_mux_get_device_count(&mgr));

    xy_mux_deinit(&mgr);

    printf("\n=== Multiple GPIO Channels Example Complete ===\n");
}

/* ==================== Main Entry ==================== */

int main(int argc, char *argv[])
{
    printf("XY_MUX GPIO Multiplexing Example\n");
    printf("Version: %s\n", XY_MUX_VERSION_STRING);
    printf("=================================\n");

    /* Run examples */
    example_basic_gpio();
    example_gpio_tlv_packet();
    example_multi_gpio();

    printf("\nAll examples completed successfully!\n");
    return 0;
}
