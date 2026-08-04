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
FAKE_VALUE_FUNC(int32_t, mock_init, uint8_t, const void *)
FAKE_VALUE_FUNC(int32_t, mock_deinit, uint8_t)

static int32_t mock_init_impl(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    return XY_MUX_OK;
}

static int32_t mock_deinit_impl(uint8_t channel)
{
    (void)channel;
    return XY_MUX_OK;
}

static int32_t mock_init_fail_impl(uint8_t channel, const void *config)
{
    (void)channel;
    (void)config;
    return XY_MUX_ERROR;
}

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
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_NULL(mgr.devices);
    TEST_ASSERT_EQUAL_UINT(0U, mgr.device_count);
    TEST_ASSERT_EQUAL_UINT(BUFFER_SIZE, mgr.buffer_size);

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
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL manager rejected\n");

    /* Test NULL tx_buffer */
    ret = xy_mux_init(&mgr, NULL, rx_buffer, BUFFER_SIZE);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL tx_buffer rejected\n");

    /* Test NULL rx_buffer */
    ret = xy_mux_init(&mgr, tx_buffer, NULL, BUFFER_SIZE);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
    printf("  [PASS] NULL rx_buffer rejected\n");

    /* Test zero buffer size */
    ret = xy_mux_init(&mgr, tx_buffer, rx_buffer, 0);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM, ret);
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
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(1U, mgr.device_count);
    printf("  [PASS] GPIO device registered\n");

    /* Register I2C device */
    ret = xy_mux_i2c_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(2U, mgr.device_count);
    printf("  [PASS] I2C device registered\n");

    /* Register SPI device */
    ret = xy_mux_spi_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(3U, mgr.device_count);
    printf("  [PASS] SPI device registered\n");

    /* Register UART device */
    ret = xy_mux_uart_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(4U, mgr.device_count);
    printf("  [PASS] UART device registered\n");

    /* Try to register duplicate (same type and channel) */
    ret = xy_mux_gpio_register(&mgr, 0, &ops, NULL);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_BUSY, ret);
    printf("  [PASS] Duplicate registration rejected\n");

    xy_mux_deinit(&mgr);
    printf("  [PASS] All devices unregistered on deinit\n");
}

static void test_mux_register_rolls_back_failed_init(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    xy_mux_ops_t ops = {
        .init = mock_init,
        .deinit = mock_deinit,
        .read = mock_read,
        .write = mock_write,
        .ioctl = mock_ioctl,
    };

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE));
    mock_init_fake.custom_fake = mock_init_fail_impl;

    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR,
                          xy_mux_register(&mgr, XY_MUX_TYPE_UART, 3, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT(1U, mock_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(3, mock_init_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(0U, mock_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mgr.device_count);
    TEST_ASSERT_NULL(mgr.devices);
    TEST_ASSERT_NULL(xy_mux_find(&mgr, XY_MUX_TYPE_UART, 3));

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_deinit(&mgr));
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
    TEST_ASSERT_EQUAL_UINT(3U, mgr.device_count);

    /* Unregister one device */
    int32_t ret = xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 1);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(2U, mgr.device_count);
    printf("  [PASS] Device unregistered successfully\n");

    /* Unregister non-existent device */
    ret = xy_mux_unregister(&mgr, XY_MUX_TYPE_GPIO, 99);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_NO_DEVICE, ret);
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
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_UINT8(5U, dev->channel);
    printf("  [PASS] Found existing device\n");

    /* Find non-existent device */
    dev = xy_mux_find(&mgr, XY_MUX_TYPE_SPI, 99);
    TEST_ASSERT_NULL(dev);
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
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(sizeof(xy_mux_header_t) + sizeof(gpio_data), packet_len);

    /* Verify packet header */
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_TYPE_GPIO, tx_buffer[0]); /* type */
    TEST_ASSERT_EQUAL_UINT8(0U, tx_buffer[1]);               /* channel */
    TEST_ASSERT_EQUAL_UINT8(sizeof(gpio_data), tx_buffer[2]); /* length low */
    TEST_ASSERT_EQUAL_UINT8(0U, tx_buffer[3]);               /* length high */
    TEST_ASSERT_EQUAL_UINT8(0x01U, tx_buffer[4]);            /* data */

    printf("  [PASS] GPIO packet built correctly (4 bytes header + 1 byte data)\n");

    /* Build I2C packet */
    uint8_t i2c_data[] = {0x50, 0xAA, 0xBB, 0xCC};
    ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_I2C, 1,
                              i2c_data, sizeof(i2c_data),
                              tx_buffer, &packet_len);
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, ret);
    TEST_ASSERT_EQUAL_UINT(sizeof(xy_mux_header_t) + sizeof(i2c_data), packet_len);

    /* Verify I2C packet header */
    TEST_ASSERT_EQUAL_UINT8(XY_MUX_TYPE_I2C, tx_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1U, tx_buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(sizeof(i2c_data), tx_buffer[2]);

    printf("  [PASS] I2C packet built correctly\n");

    /* Test packet too large */
    uint8_t large_data[600];
    ret = xy_mux_build_packet(&mgr, XY_MUX_TYPE_SPI, 0,
                              large_data, sizeof(large_data),
                              tx_buffer, &packet_len);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_NO_MEMORY, ret);
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
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(gpio_data), ret);
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
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_NO_DEVICE, ret);
    printf("  [PASS] Non-existent device packet rejected\n");

    xy_mux_deinit(&mgr);
}

static void test_process_packet_rejects_header_length_mismatch(void)
{
    xy_mux_manager_t mgr;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t gpio_data[] = {0x55, 0xAA};
    size_t packet_len = 0;
    xy_mux_ops_t ops = {
        .write = mock_write,
    };

    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_init(&mgr, tx_buffer, rx_buffer, BUFFER_SIZE));
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_gpio_register(&mgr, 0, &ops, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MUX_OK, xy_mux_build_packet(&mgr, XY_MUX_TYPE_GPIO, 0, gpio_data,
                                                         sizeof(gpio_data), tx_buffer,
                                                         &packet_len));

    xy_mux_header_t *header = (xy_mux_header_t *)tx_buffer;

    header->length = (uint16_t)(sizeof(gpio_data) + 1U);
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM,
                          xy_mux_process_packet(&mgr, tx_buffer, packet_len));
    TEST_ASSERT_EQUAL_UINT(0U, mock_write_fake.call_count);

    header->length = 1U;
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_INVALID_PARAM,
                          xy_mux_process_packet(&mgr, tx_buffer, packet_len));
    TEST_ASSERT_EQUAL_UINT(0U, mock_write_fake.call_count);

    header->length = (uint16_t)sizeof(gpio_data);
    TEST_ASSERT_EQUAL_INT((int32_t)sizeof(gpio_data),
                          xy_mux_process_packet(&mgr, tx_buffer, packet_len));
    TEST_ASSERT_EQUAL_UINT(1U, mock_write_fake.call_count);

    xy_mux_deinit(&mgr);
}

/**
 * @brief Test type string conversion
 */
static void test_type_string_conversion(void)
{
    printf("\n[Test] Type String Conversion\n");

    /* Test valid types */
    TEST_ASSERT_EQUAL_STRING("GPIO", xy_mux_type_to_string(XY_MUX_TYPE_GPIO));
    TEST_ASSERT_EQUAL_STRING("I2C", xy_mux_type_to_string(XY_MUX_TYPE_I2C));
    TEST_ASSERT_EQUAL_STRING("SPI", xy_mux_type_to_string(XY_MUX_TYPE_SPI));
    TEST_ASSERT_EQUAL_STRING("UART", xy_mux_type_to_string(XY_MUX_TYPE_UART));
    printf("  [PASS] Type to string conversion works\n");

    /* Test string to type */
    TEST_ASSERT_EQUAL_INT(XY_MUX_TYPE_GPIO, xy_mux_string_to_type("GPIO"));
    TEST_ASSERT_EQUAL_INT(XY_MUX_TYPE_I2C, xy_mux_string_to_type("I2C"));
    TEST_ASSERT_EQUAL_INT(XY_MUX_TYPE_SPI, xy_mux_string_to_type("SPI"));
    TEST_ASSERT_EQUAL_INT(XY_MUX_TYPE_UART, xy_mux_string_to_type("UART"));
    printf("  [PASS] String to type conversion works\n");

    /* Test invalid string */
    TEST_ASSERT_EQUAL_INT(XY_MUX_TYPE_CUSTOM, xy_mux_string_to_type("INVALID"));
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
    TEST_ASSERT_EQUAL_UINT16(4U, count);
    printf("  [PASS] Retrieved %d devices\n", count);

    /* Test with small buffer */
    xy_mux_device_t *small_list[2];
    count = xy_mux_get_device_list(&mgr, small_list, 2);
    TEST_ASSERT_EQUAL_UINT16(2U, count);
    printf("  [PASS] Limited to %d devices due to buffer size\n", count);

    /* Test NULL parameters */
    count = xy_mux_get_device_list(&mgr, NULL, 8);
    TEST_ASSERT_EQUAL_UINT16(0U, count);
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
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_NOT_SUPPORTED, ret);
    printf("  [PASS] Read on device without read op returns NOT_SUPPORTED\n");

    /* Test write on device without write operation */
    ret = xy_mux_write(&mgr, XY_MUX_TYPE_GPIO, 0, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_MUX_ERROR_NOT_SUPPORTED, ret);
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
    RESET_FAKE(mock_init);
    RESET_FAKE(mock_deinit);
    FFF_RESET_HISTORY();

    mock_write_fake.custom_fake = mock_write_impl;
    mock_read_fake.custom_fake = mock_read_impl;
    mock_ioctl_fake.custom_fake = mock_ioctl_impl;
    mock_init_fake.custom_fake = mock_init_impl;
    mock_deinit_fake.custom_fake = mock_deinit_impl;
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
    RUN_TEST(test_mux_register_rolls_back_failed_init);
    RUN_TEST(test_mux_unregister);
    RUN_TEST(test_mux_find);
    RUN_TEST(test_build_packet);
    RUN_TEST(test_process_packet);
    RUN_TEST(test_process_packet_rejects_header_length_mismatch);
    RUN_TEST(test_type_string_conversion);
    RUN_TEST(test_get_device_list);
    RUN_TEST(test_read_write);
    return UNITY_END();
}
