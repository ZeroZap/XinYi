/**
 * @file test_device_bus_repeated_start.c
 * @brief The canonical I2C register read has two wire shapes, and SMBus needs the other one.
 *
 * xy_i2c_device_read_reg() writes the register pointer and then reads, as two HAL calls. On the wire
 * that is a STOP between the write and the read. An SMBus fuel gauge does not accept that: its register
 * read is one transaction with a repeated START, which the HAL exposes as xy_hal_i2c_mem_read().
 *
 * The legacy sensor bus helper used xy_hal_i2c_mem_read() for exactly this reason, which is why the
 * fuel-gauge drivers could not simply be pointed at the canonical helper: the types line up and the
 * wire does not. This suite exists so the two shapes are a measured property of the code rather than a
 * paragraph in a plan: the helpers are linked against counting HAL stubs, so the number and kind of
 * transfers each one issues is asserted, not assumed.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "xy_hal_uart.h"

/* -------------------------------------------------------------------------------------------------
 * Counting HAL stubs. Only components/device/src/xy_device_bus_helpers.c is linked into this binary,
 * so these replace the backend instead of fighting it.
 * ---------------------------------------------------------------------------------------------- */

static int g_transmit_calls;
static int g_receive_calls;
static int g_mem_read_calls;
static int g_mem_write_calls;

static xy_hal_error_t g_next_status;
static uint8_t g_mem_read_reply[4];

static uint16_t g_last_mem_write_addr;
static uint16_t g_last_mem_write_reg;
static size_t g_last_mem_write_len;
static uint8_t g_last_mem_write_payload[8];

static uint16_t g_last_mem_read_addr;
static uint16_t g_last_mem_read_reg;
static size_t g_last_mem_read_len;
static uint32_t g_last_mem_read_timeout;
static void *g_last_mem_read_handle;

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)i2c;
    (void)dev_addr;
    (void)data;
    (void)len;
    (void)timeout;
    g_transmit_calls++;
    return g_next_status;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                         uint8_t *data, size_t len, uint32_t timeout)
{
    (void)i2c;
    (void)dev_addr;
    (void)timeout;
    g_receive_calls++;
    if (g_next_status == XY_HAL_OK && data != NULL && len > 0U) {
        data[0] = 0x5AU;
    }
    return g_next_status;
}

xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                   uint8_t *data, size_t len, uint32_t timeout)
{
    g_mem_read_calls++;
    g_last_mem_read_handle = i2c;
    g_last_mem_read_addr = dev_addr;
    g_last_mem_read_reg = reg_addr;
    g_last_mem_read_len = len;
    g_last_mem_read_timeout = timeout;

    if (g_next_status != XY_HAL_OK) {
        return g_next_status;
    }
    if (data != NULL) {
        for (size_t index = 0U; index < len && index < sizeof(g_mem_read_reply); index++) {
            data[index] = g_mem_read_reply[index];
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)i2c;
    (void)timeout;
    g_mem_write_calls++;
    g_last_mem_write_addr = dev_addr;
    g_last_mem_write_reg = reg_addr;
    g_last_mem_write_len = len;
    for (size_t index = 0U; index < len && index < sizeof(g_last_mem_write_payload); index++) {
        g_last_mem_write_payload[index] = data[index];
    }
    return g_next_status;
}

/* -------------------------------------------------------------------------------------------------
 * The rest of the bus-helper file links against symbol the I2C cases never reach: SPI, UART and GPIO
 * transfers, and the device registry. They are stubbed rather than linked so this binary stays small and
 * the HAL is entirely local. Prototypes come from the HAL headers, so these cannot drift from them:
 * a signature change breaks the build here instead of at a link step nobody reads.
 * ---------------------------------------------------------------------------------------------- */

int xy_device_registry_register(xy_device_t *dev)
{
    (void)dev;
    return 0;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)spi; (void)data; (void)len; (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)spi; (void)data; (void)len; (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data, uint8_t *rx_data,
                                           size_t len, uint32_t timeout)
{
    (void)spi; (void)tx_data; (void)rx_data; (void)len; (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)uart; (void)data; (void)len; (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)uart; (void)data; (void)len; (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    (void)port; (void)pin; (void)config;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    (void)port; (void)pin; (void)value;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port; (void)pin;
    return 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port; (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

/* --------------------------------------------------------------------------------------------- */

static xy_i2c_device_t g_device;
static int g_fake_handle = 7;

void setUp(void)
{
    memset(&g_device, 0, sizeof(g_device));
    memset(g_mem_read_reply, 0, sizeof(g_mem_read_reply));

    g_transmit_calls = 0;
    g_receive_calls = 0;
    g_mem_read_calls = 0;
    g_mem_write_calls = 0;
    g_last_mem_write_addr = 0U;
    g_last_mem_write_reg = 0U;
    g_last_mem_write_len = 0U;
    memset(g_last_mem_write_payload, 0, sizeof(g_last_mem_write_payload));
    g_next_status = XY_HAL_OK;

    g_last_mem_read_addr = 0U;
    g_last_mem_read_reg = 0U;
    g_last_mem_read_len = 0U;
    g_last_mem_read_timeout = 0U;
    g_last_mem_read_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_init(&g_device, &g_fake_handle, 0x0BU, 250U));
}

void tearDown(void)
{
}

/* === the shape this suite exists for ============================================================ */

/* The existing canonical read is two transfers. Recorded, not presumed: if this ever becomes one call
 * the difference that blocked the fuel gauges has silently disappeared and someone should know. */
static void test_plain_register_read_is_two_transfers(void)
{
    uint8_t data = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_read_reg(&g_device, 0x09U, &data, 1U));
    TEST_ASSERT_EQUAL_INT(1, g_transmit_calls);
    TEST_ASSERT_EQUAL_INT(1, g_receive_calls);
    TEST_ASSERT_EQUAL_INT(0, g_mem_read_calls);
}

/* SMBus registers: one transaction, one repeated START. */
static void test_repeated_start_read_is_one_transaction(void)
{
    uint8_t data[2] = {0U, 0U};

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_read_reg_repeated_start(&g_device, 0x0DU, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(1, g_mem_read_calls);
    TEST_ASSERT_EQUAL_INT(0, g_transmit_calls);
    TEST_ASSERT_EQUAL_INT(0, g_receive_calls);
}

static void test_repeated_start_read_forwards_address_register_length_and_timeout(void)
{
    uint8_t data = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_read_reg_repeated_start(&g_device, 0x3EU, &data, 1U));
    TEST_ASSERT_EQUAL_HEX16(0x0BU, g_last_mem_read_addr);
    TEST_ASSERT_EQUAL_HEX16(0x3EU, g_last_mem_read_reg);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)g_last_mem_read_len);
    TEST_ASSERT_EQUAL_UINT32(250U, g_last_mem_read_timeout);
    TEST_ASSERT_EQUAL_PTR(&g_fake_handle, g_last_mem_read_handle);
}

/* A 16-bit SMBus word is least significant byte first. Reversing it silently scrambles every value the
 * gauge reports, which is the kind of defect that reads as a calibration problem for weeks. */
static void test_repeated_start_word_read_assembles_least_significant_byte_first(void)
{
    uint16_t value = 0U;

    g_mem_read_reply[0] = 0x34U; /* low byte arrives first */
    g_mem_read_reply[1] = 0x12U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_read_reg16_repeated_start(&g_device, 0x09U, &value));
    TEST_ASSERT_EQUAL_HEX16(0x1234U, value);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)g_last_mem_read_len);
}

/* The write side has the same split: an SMBus command write is one transaction. */
static void test_repeated_start_write_is_one_transaction(void)
{
    const uint8_t payload[2] = {0x11U, 0x22U};

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_write_reg_repeated_start(&g_device, 0x14U, payload,
                                                                 sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(1, g_mem_write_calls);
    TEST_ASSERT_EQUAL_HEX16(0x0BU, g_last_mem_write_addr);
    TEST_ASSERT_EQUAL_HEX16(0x14U, g_last_mem_write_reg);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)g_last_mem_write_len);
    TEST_ASSERT_EQUAL_HEX8(0x11U, g_last_mem_write_payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22U, g_last_mem_write_payload[1]);
}

static void test_repeated_start_write_refuses_without_transferring(void)
{
    const uint8_t payload = 0x01U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_write_reg_repeated_start(&g_device, 0x14U, NULL, 1U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_write_reg_repeated_start(&g_device, 0x14U, &payload, 0U));
    TEST_ASSERT_EQUAL_INT(0, g_mem_write_calls);
}

static void test_a_failed_repeated_start_write_reports_an_io_error(void)
{
    const uint8_t payload = 0x01U;

    g_next_status = XY_HAL_ERROR_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_i2c_device_write_reg_repeated_start(&g_device, 0x14U, &payload, 1U));
}

/* === refusals happen before the bus is touched ================================================== */

static void test_repeated_start_read_refuses_a_null_buffer_without_transferring(void)
{
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_read_reg_repeated_start(&g_device, 0x09U, NULL, 1U));
    TEST_ASSERT_EQUAL_INT(0, g_mem_read_calls);
}

static void test_repeated_start_read_refuses_zero_length_without_transferring(void)
{
    uint8_t data = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_read_reg_repeated_start(&g_device, 0x09U, &data, 0U));
    TEST_ASSERT_EQUAL_INT(0, g_mem_read_calls);
}

static void test_repeated_start_read_refuses_an_uninitialised_device_without_transferring(void)
{
    xy_i2c_device_t cold;
    uint8_t data = 0U;

    memset(&cold, 0, sizeof(cold));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_read_reg_repeated_start(&cold, 0x09U, &data, 1U));
    TEST_ASSERT_EQUAL_INT(0, g_mem_read_calls);
}

static void test_repeated_start_word_read_refuses_a_null_value_without_transferring(void)
{
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_read_reg16_repeated_start(&g_device, 0x09U, NULL));
    TEST_ASSERT_EQUAL_INT(0, g_mem_read_calls);
}

/* === a failed transfer is reported, not swallowed =============================================== */

static void test_a_failed_repeated_start_read_reports_an_io_error(void)
{
    uint8_t data = 0U;

    g_next_status = XY_HAL_ERROR_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_i2c_device_read_reg_repeated_start(&g_device, 0x09U, &data, 1U));
}

static void test_a_failed_repeated_start_word_read_reports_an_io_error_and_leaves_the_value_alone(void)
{
    uint16_t value = 0xBEEFU;

    g_next_status = XY_HAL_ERROR;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_i2c_device_read_reg16_repeated_start(&g_device, 0x09U, &value));
    TEST_ASSERT_EQUAL_HEX16(0xBEEFU, value);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_plain_register_read_is_two_transfers);
    RUN_TEST(test_repeated_start_read_is_one_transaction);
    RUN_TEST(test_repeated_start_read_forwards_address_register_length_and_timeout);
    RUN_TEST(test_repeated_start_word_read_assembles_least_significant_byte_first);
    RUN_TEST(test_repeated_start_write_is_one_transaction);
    RUN_TEST(test_repeated_start_write_refuses_without_transferring);
    RUN_TEST(test_a_failed_repeated_start_write_reports_an_io_error);
    RUN_TEST(test_repeated_start_read_refuses_a_null_buffer_without_transferring);
    RUN_TEST(test_repeated_start_read_refuses_zero_length_without_transferring);
    RUN_TEST(test_repeated_start_read_refuses_an_uninitialised_device_without_transferring);
    RUN_TEST(test_repeated_start_word_read_refuses_a_null_value_without_transferring);
    RUN_TEST(test_a_failed_repeated_start_read_reports_an_io_error);
    RUN_TEST(test_a_failed_repeated_start_word_read_reports_an_io_error_and_leaves_the_value_alone);

    return UNITY_END();
}
