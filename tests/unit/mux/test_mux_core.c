/**
 * @file test_mux_core.c
 * @brief Test TLV Packet Building and Processing
 * @version 1.0.0
 * @date 2026-03-02
 *
 * Unit tests for core MUX functionality including:
 * - TLV packet building
 * - TLV packet parsing/processing
 * - Device registration and unregistration
 * - Manager initialization/deinitialization
 */

#include "xy_mux.h"
#include "xy_mux_gpio.h"
#include "xy_mux_i2c.h"
#include "xy_mux_spi.h"
#include "xy_mux_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fff.h"
#include "unity.h"

DEFINE_FFF_GLOBALS;

void xy_log_char(char ch)
{
    (void)ch;
}

/* Test buffer size */
#define BUFFER_SIZE 512

FAKE_VALUE_FUNC(int32_t, mock_write, uint8_t, const void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_read, uint8_t, void *, size_t)
FAKE_VALUE_FUNC(int32_t, mock_ioctl, uint8_t, int, void *)

static int32_t mock_write_impl(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    (void)data;
    return (int32_t)len;
}

static int32_t mock_read_impl(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    (void)data;
    return (int32_t)len;
}

static int32_t mock_ioctl_impl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    (void)cmd;
    (void)arg;
    return 0;
}

/* ==================== Test Cases ==================== */

/**
 * @brief Test manager initialization
 */
static void test_mux_init(void)
{
    printf("\n[Test] MUX Manager Initialization\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    int32_t ret = xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(mgr.devices == NULL);
    TEST_ASSERT_TRUE(mgr.device_count == 0);
    TEST_ASSERT_TRUE(mgr.buffer_size == BUFFER_SIZE);

    printf("  [PASS] Manager initialized successfully\n");

    xy_mux_deinit(&mgr);
    printf("  [PASS] Manager deinitialized successfully\n");
}

/**
 * @brief Test manager initialization with invalid parameters
 */
static void test_mux_init_invalid(void)
{
    printf("\n[Test] MUX Manager Initialization with Invalid Parameters\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];

    /* Test NULL manager */
    int32_t ret = xy_mux_init(NULL, tx_buffer, rx_buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL manager rejected\n");

    /* Test NULL tx_buffer */
    ret = xy_mux_init(&mgr, NULL, rx_buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL tx_buffer rejected\n");

    /* Test NULL rx_buffer */
    ret = xy_mux_init(&mgr, tx_buffer, NULL, BUFFER_SIZE);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] NULL rx_buffer rejected\n");

    /* Test zero buffer size */
    ret = xy_mux_init(&mgr, tx_buffer, rx_buffer, 0);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_INVALID_PARAM);
    printf("  [PASS] Zero buffer size rejected\n");
}

/**
 * @brief Test device registration
 */
static void test_mux_register(void)
{
    printf("\n[Test] Device Registration\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .init = NULL,
        .deinit = NULL,
        .read = mock_read,
        .write = mock_write,
        .ioctl = mock_ioctl,
    };

    /* Register GPIO device */
    int32_t ret = xy_mux_gpio_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(mgr.device_count == 1);
    printf("  [PASS] GPIO device registered\n");

    /* Register I2C device */
    ret = xy_mux_i2c_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(mgr.device_count == 2);
    printf("  [PASS] I2C device registered\n");

    /* Register SPI device */
    ret = xy_mux_spi_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(mgr.device_count == 3);
    printf("  [PASS] SPI device registered\n");

    /* Register UART device */
    ret = xy_mux_uart_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(mgr.device_count == 4);
    printf("  [PASS] UART device registered\n");

    /* Try to register duplicate (same type and channel) */
    ret = xy_mux_gpio_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_BUSY);
    printf("  [PASS] Duplicate registration rejected\n");

    xy_mux_deinit(&mgr);
    printf("  [PASS] All devices unregistered on deinit\n");
}

/**
 * @brief Test device unregistration
 */
static void test_mux_unregister(void)
{
    printf("\n[Test] Device Unregistration\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_read,
        .write = mock_write,
        .ioctl = mock_ioctl,
    };

    /* Register devices */
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);
    xy_mux_gpio_register(&mgr, 1, &ops, NULL);
    xy_mux_gpio_register(&mgr, 2, &ops, NULL);
    TEST_ASSERT_TRUE(mgr.device_count == 3);

    /* Unregister one device */
    int32_t ret = xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 1);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(mgr.device_count == 2);
    printf("  [PASS] Device unregistered successfully\n");

    /* Unregister non-existent device */
    ret = xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 99);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_NO_DEVICE);
    printf("  [PASS] Non-existent device unregister rejected\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test device finding
 */
static void test_mux_find(void)
{
    printf("\n[Test] Device Finding\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_read,
        .write = mock_write,
        .ioctl = mock_ioctl,
    };

    /* Register devices */
    xy_mux_gpio_register(&mgr, 5, &ops, NULL);
    xy_mux_i2c_register(&mgr, 3, &ops, NULL);

    /* Find existing device */
    xy_mux_device_t *dev = xy_mux_find(&mgr, XY_MUX_TYPE_GPIO, 5);
    TEST_ASSERT_TRUE(dev != NULL);
    TEST_ASSERT_TRUE(dev->channel == 5);
    printf("  [PASS] Found existing device\n");

    /* Find non-existent device */
    dev = xy_mux_find(&mgr, XY_MUX_TYPE_SPI, 99);
    TEST_ASSERT_TRUE(dev == NULL);
    printf("  [PASS] Non-existent device returns NULL\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test TLV packet building
 */
static void test_build_packet(void)
{
    printf("\n[Test] TLV Packet Building\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Build GPIO packet */
    uint8_t gpio_data = 0x01;
    size_t packet_len = 0;

    int32_t ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_GPIO, 0,
                                      &gpio_data, sizeof(gpio_data),
                                      tx_buffer, &packet_len);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(packet_len == sizeof(xy_mux_header_t) + sizeof(gpio_data));

    /* Verify packet header */
    TEST_ASSERT_TRUE(tx_buffer[0] == XY_MUX_TYPE_GPIO);  /* type */
    TEST_ASSERT_TRUE(tx_buffer[1] == 0);                 /* channel */
    TEST_ASSERT_TRUE(tx_buffer[2] == sizeof(gpio_data)); /* length low */
    TEST_ASSERT_TRUE(tx_buffer[3] == 0);                 /* length high */
    TEST_ASSERT_TRUE(tx_buffer[4] == 0x01);              /* data */

    printf("  [PASS] GPIO packet built correctly (4 bytes header + 1 byte data)\n");

    /* Build I2C packet */
    uint8_t i2c_data[] = {0x50, 0xAA, 0xBB, 0xCC};
    ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_I2C, 1,
                              i2c_data, sizeof(i2c_data),
                              tx_buffer, &packet_len);
    TEST_ASSERT_TRUE(ret == XY_MUX_OK);
    TEST_ASSERT_TRUE(packet_len == sizeof(xy_mux_header_t) + sizeof(i2c_data));

    /* Verify I2C packet header */
    TEST_ASSERT_TRUE(tx_buffer[0] == XY_MUX_TYPE_I2C);
    TEST_ASSERT_TRUE(tx_buffer[1] == 1);
    TEST_ASSERT_TRUE(tx_buffer[2] == sizeof(i2c_data));

    printf("  [PASS] I2C packet built correctly\n");

    /* Test packet too large */
    uint8_t large_data[600];
    ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_SPI, 0,
                              large_data, sizeof(large_data),
                              tx_buffer, &packet_len);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_NO_MEMORY);
    printf("  [PASS] Oversized packet rejected\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test packet processing
 */
static void test_process_packet(void)
{
    printf("\n[Test] TLV Packet Processing\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Register GPIO device to receive packets */
    xy_mux_ops_t ops = {
        .write = mock_write,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Build a packet */
    uint8_t gpio_data = 0x55;
    size_t packet_len = 0;
    xy_mux_build_packet(&mgr, XY_MUX_TYPE_GPIO, 0,
                        &gpio_data, sizeof(gpio_data),
                        tx_buffer, &packet_len);

    /* Process the packet */
    int32_t ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    TEST_ASSERT_TRUE(ret == (int32_t)sizeof(gpio_data));
    TEST_ASSERT_EQUAL_UINT(1U, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_write_fake.arg0_val);
    TEST_ASSERT_NOT_NULL(mock_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_MEMORY(&gpio_data, mock_write_fake.arg1_val, sizeof(gpio_data));
    TEST_ASSERT_EQUAL_UINT32(sizeof(gpio_data), mock_write_fake.arg2_val);
    printf("  [PASS] Packet processed successfully\n");

    /* Test processing with non-existent device */
    xy_mux_build_packet(&mgr, XY_MUX_TYPE_GPIO, 99,
                        &gpio_data, sizeof(gpio_data),
                        tx_buffer, &packet_len);
    ret = xy_mux_process_packet(&mgr, tx_buffer, packet_len);
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_NO_DEVICE);
    printf("  [PASS] Non-existent device packet rejected\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test type string conversion
 */
static void test_type_string_conversion(void)
{
    printf("\n[Test] Type String Conversion\n");

    /* Test valid types */
    TEST_ASSERT_TRUE(strcmp(xy_mux_type_to_string(XY_MUX_TYPE_GPIO), "GPIO") == 0);
    TEST_ASSERT_TRUE(strcmp(xy_mux_type_to_string(XY_MUX_TYPE_I2C), "I2C") == 0);
    TEST_ASSERT_TRUE(strcmp(xy_mux_type_to_string(XY_MUX_TYPE_SPI), "SPI") == 0);
    TEST_ASSERT_TRUE(strcmp(xy_mux_type_to_string(XY_MUX_TYPE_UART), "UART") == 0);
    printf("  [PASS] Type to string conversion works\n");

    /* Test string to type */
    TEST_ASSERT_TRUE(xy_mux_string_to_type("GPIO") == XY_MUX_TYPE_GPIO);
    TEST_ASSERT_TRUE(xy_mux_string_to_type("I2C") == XY_MUX_TYPE_I2C);
    TEST_ASSERT_TRUE(xy_mux_string_to_type("SPI") == XY_MUX_TYPE_SPI);
    TEST_ASSERT_TRUE(xy_mux_string_to_type("UART") == XY_MUX_TYPE_UART);
    printf("  [PASS] String to type conversion works\n");

    /* Test invalid string */
    TEST_ASSERT_TRUE(xy_mux_string_to_type("INVALID") == XY_MUX_TYPE_CUSTOM);
    printf("  [PASS] Invalid string returns CUSTOM type\n");
}

/**
 * @brief Test device list retrieval
 */
static void test_get_device_list(void)
{
    printf("\n[Test] Device List Retrieval\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    xy_mux_ops_t ops = {
        .read = mock_read,
        .write = mock_write,
        .ioctl = mock_ioctl,
    };

    /* Register multiple devices */
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);
    xy_mux_gpio_register(&mgr, 1, &ops, NULL);
    xy_mux_i2c_register(&mgr, 0, &ops, NULL);
    xy_mux_spi_register(&mgr, 0, &ops, NULL);

    /* Get device list */
    xy_mux_device_t *devices[8];
    uint16_t count = xy_mux_get_device_list(&mgr, devices, 8);
    TEST_ASSERT_TRUE(count == 4);
    printf("  [PASS] Retrieved %d devices\n", count);

    /* Test with small buffer */
    xy_mux_device_t *small_list[2];
    count = xy_mux_get_device_list(&mgr, small_list, 2);
    TEST_ASSERT_TRUE(count == 2);
    printf("  [PASS] Limited to %d devices due to buffer size\n", count);

    /* Test NULL parameters */
    count = xy_mux_get_device_list(&mgr, NULL, 8);
    TEST_ASSERT_TRUE(count == 0);
    printf("  [PASS] NULL devices array returns 0\n");

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test read/write operations
 */
static void test_read_write(void)
{
    printf("\n[Test] Read/Write Operations\n");

    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE);

    /* Register device without read/write ops */
    xy_mux_ops_t ops = {
        .read = NULL,
        .write = NULL,
        .ioctl = mock_ioctl,
    };
    xy_mux_gpio_register(&mgr, 0, &ops, NULL);

    /* Test read on device without read operation */
    uint8_t data[4];
    int32_t ret = xy_mux_read(&mgr, XY_MUX_TYPE_GPIO, 0, data, sizeof(data));
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_NOT_SUPPORTED);
    printf("  [PASS] Read on device without read op returns NOT_SUPPORTED\n");

    /* Test write on device without write operation */
    ret = xy_mux_write(&mgr, XY_MUX_TYPE_GPIO, 0, data, sizeof(data));
    TEST_ASSERT_TRUE(ret == XY_MUX_ERROR_NOT_SUPPORTED);
    TEST_ASSERT_EQUAL_UINT(0U, mock_ioctl_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_write_fake.call_count);
    printf("  [PASS] Write on device without write op returns NOT_SUPPORTED\n");

    xy_mux_deinit(&mgr);
}

/* ==================== Main Entry ==================== */

void setUp(void)
{
    RESET_FAKE(mock_write);
    RESET_FAKE(mock_read);
    RESET_FAKE(mock_ioctl);
    FFF_RESET_HISTORY();

    mock_write_fake.custom_fake = mock_write_impl;
    mock_read_fake.custom_fake = mock_read_impl;
    mock_ioctl_fake.custom_fake = mock_ioctl_impl;
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mux_init);
    RUN_TEST(test_mux_init_invalid);
    RUN_TEST(test_mux_register);
    RUN_TEST(test_mux_unregister);
    RUN_TEST(test_mux_find);
    RUN_TEST(test_build_packet);
    RUN_TEST(test_process_packet);
    RUN_TEST(test_type_string_conversion);
    RUN_TEST(test_get_device_list);
    RUN_TEST(test_read_write);
    return UNITY_END();
}
