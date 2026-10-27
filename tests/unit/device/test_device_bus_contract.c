/**
 * @file test_device_bus_contract.c
 * @brief Canonical return-value contract for the device bus helpers.
 *
 * xy_device.h declares the bus helpers with xy_error_t returns
 * (XY_DEVICE_OK == 0 / XY_DEVICE_IO_ERROR / XY_DEVICE_INVALID_PARAM).
 * Real consumers compare the result against XY_DEVICE_OK before issuing the
 * next transfer (see components/sensor/src/xy_w25qxx.c and
 * components/drivers/sensor/motion/adxl362/xy_adxl362.c), so a helper that
 * returns the transferred byte count on success is indistinguishable from a
 * failure and silently aborts the caller's transfer sequence.
 *
 * The byte-count shape is preserved where it is actually part of the API:
 * xy_spi_device_send()/xy_spi_device_recv() return the number of bytes.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"
#include "xy_os.h"
#include "xy_w25qxx.h"

static int g_fake_bus = 1;
static int g_fake_cs = 3;

/* Link-time stubs for the driver side deps of xy_w25qxx.c (no CLIB/OSAL here). */
int xy_printf(const char* fmt, ...) {
    (void)fmt;
    return 0;
}

xy_os_status_t xy_os_delay(uint32_t ticks) {
    (void)ticks;
    return XY_OS_OK;
}

uint32_t xy_os_tick_get(void) {
    return 0U;
}

void setUp(void) {}

void tearDown(void) {}

static void init_spi(xy_spi_device_t* dev) {
    memset(dev, 0, sizeof(*dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_spi_device_init(dev, &g_fake_bus, &g_fake_cs, 1000000U, 0U));
}

/* ---------- xy_spi_device_transfer ---------- */

static void test_spi_transfer_success_returns_xy_device_ok(void) {
    xy_spi_device_t dev;
    uint8_t tx[3] = {0x90U, 0x00U, 0x00U};
    uint8_t rx[3] = {0};

    init_spi(&dev);

    /* full duplex */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_transfer(&dev, tx, rx, sizeof(tx)));
    /* tx only */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_transfer(&dev, tx, NULL, sizeof(tx)));
    /* rx only */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_transfer(&dev, NULL, rx, sizeof(rx)));
}

static void test_spi_transfer_rejects_null_buffers(void) {
    xy_spi_device_t dev;

    init_spi(&dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_spi_device_transfer(&dev, NULL, NULL, 4U));
}

static void test_spi_transfer_rejects_uninitialised_device(void) {
    xy_spi_device_t dev;
    uint8_t tx[2] = {0x01U, 0x02U};
    uint8_t rx[2] = {0};

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_spi_device_transfer(&dev, tx, rx, sizeof(tx)));
}

/* ---------- xy_spi_device_send / xy_spi_device_recv ---------- */

static void test_spi_send_recv_report_byte_count(void) {
    xy_spi_device_t dev;
    uint8_t tx[4] = {0x03U, 0xA0U, 0x00U, 0x00U};
    uint8_t rx[4] = {0};

    init_spi(&dev);

    TEST_ASSERT_EQUAL_INT(4, xy_spi_device_send(&dev, tx, sizeof(tx)));
    TEST_ASSERT_EQUAL_INT(4, xy_spi_device_recv(&dev, rx, sizeof(rx)));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_spi_device_send(&dev, NULL, 4U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_spi_device_recv(&dev, NULL, 4U));
}

/* ---------- UART helper return contract ---------- */

static void test_uart_send_recv_return_xy_device_ok(void) {
    xy_uart_device_t dev;
    uint8_t payload[3] = {0xAAU, 0xBBU, 0xCCU};
    uint8_t incoming[2] = {0};

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_uart_device_init(&dev, &g_fake_bus, 115200U));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_uart_device_send(&dev, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_uart_device_recv(&dev, incoming, sizeof(incoming)));
}

static void test_uart_printf_returns_written_length(void) {
    xy_uart_device_t dev;

    memset(&dev, 0, sizeof(dev));
    xy_uart_device_init(&dev, &g_fake_bus, 115200U);

    TEST_ASSERT_EQUAL_INT(5, xy_uart_device_printf(&dev, "ABCDE"));
}

/* ---------- real consumer: xy_w25qxx read-id sequence ---------- */

/* xy_w25qxx_read_id() issues "write cmd -> write addr -> read id" and checks
 * every helper result against XY_DEVICE_OK. With a byte-count return contract
 * the very first transfer aborts the sequence, so the canonical helper makes
 * this real driver path work end to end. */
static void test_w25qxx_read_id_completes_with_canonical_helper(void) {
    static xy_w25qxx_t flash;
    uint8_t manufacturer_id = 0xEEU;
    uint8_t device_id = 0xEEU;

    memset(&flash, 0, sizeof(flash));
    flash.cs_pin = 4U;
    TEST_ASSERT_EQUAL_INT(
        XY_DEVICE_OK, xy_spi_device_init(&flash.spi_dev, &g_fake_bus, &g_fake_cs, 1000000U, 0U));

    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_read_id(&flash, &manufacturer_id, &device_id));
    /* PC HAL returns zeroed RX data; the values must have been written. */
    TEST_ASSERT_EQUAL_UINT8(0x00U, manufacturer_id);
    TEST_ASSERT_EQUAL_UINT8(0x00U, device_id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_spi_transfer_success_returns_xy_device_ok);
    RUN_TEST(test_spi_transfer_rejects_null_buffers);
    RUN_TEST(test_spi_transfer_rejects_uninitialised_device);
    RUN_TEST(test_spi_send_recv_report_byte_count);
    RUN_TEST(test_uart_send_recv_return_xy_device_ok);
    RUN_TEST(test_uart_printf_returns_written_length);
    RUN_TEST(test_w25qxx_read_id_completes_with_canonical_helper);

    return UNITY_END();
}
