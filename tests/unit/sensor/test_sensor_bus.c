/**
 * @file test_sensor_bus.c
 * @brief Host contract tests for the sensor bus access layer (`components/sensor/sensor_bus.c`).
 *
 * The point of these cases is the boundary, not the happy path: a caller that hands over an
 * unconfigured bus, a missing handle, a shifted I2C address or a buffer it does not own must get
 * `-1` and the HAL must not be touched. The earlier implementation could not be reached from any
 * build at all, so nothing here was ever observed.
 */

#include "unity.h"

#include <string.h>

#include "xy_sensor.h"
#include "xy_hal_i2c.h"
#include "xy_hal_spi.h"

/* ==================== HAL fakes ==================== */

#define FAKE_TX_MAX 8

static xy_hal_error_t fake_i2c_read_status;
static xy_hal_error_t fake_i2c_write_status;
static xy_hal_error_t fake_spi_tx_status;
static xy_hal_error_t fake_spi_rx_status;

static int fake_i2c_read_calls;
static int fake_i2c_write_calls;
static int fake_spi_tx_calls;
static int fake_spi_rx_calls;

static void *fake_last_handle;
static uint16_t fake_last_addr;
static uint16_t fake_last_reg;
static size_t fake_last_len;
static uint32_t fake_last_timeout;
static uint8_t fake_tx[FAKE_TX_MAX];
static uint8_t fake_tx_count[FAKE_TX_MAX];
static size_t fake_tx_lens[FAKE_TX_MAX];

static uint8_t fake_read_bytes[4];

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

static void fake_reset(void)
{
    fake_i2c_read_status = XY_HAL_OK;
    fake_i2c_write_status = XY_HAL_OK;
    fake_spi_tx_status = XY_HAL_OK;
    fake_spi_rx_status = XY_HAL_OK;

    fake_i2c_read_calls = 0;
    fake_i2c_write_calls = 0;
    fake_spi_tx_calls = 0;
    fake_spi_rx_calls = 0;

    fake_last_handle = NULL;
    fake_last_addr = 0;
    fake_last_reg = 0;
    fake_last_len = 0;
    fake_last_timeout = 0;

    memset(fake_tx, 0, sizeof(fake_tx));
    memset(fake_tx_count, 0, sizeof(fake_tx_count));
    memset(fake_tx_lens, 0, sizeof(fake_tx_lens));
    memset(fake_read_bytes, 0, sizeof(fake_read_bytes));
}

xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                   uint8_t *data, size_t len, uint32_t timeout)
{
    fake_i2c_read_calls++;
    fake_last_handle = i2c;
    fake_last_addr = dev_addr;
    fake_last_reg = reg_addr;
    fake_last_len = len;
    fake_last_timeout = timeout;

    if (fake_i2c_read_status != XY_HAL_OK) {
        return fake_i2c_read_status;
    }

    for (size_t i = 0; i < len && i < sizeof(fake_read_bytes); i++) {
        data[i] = fake_read_bytes[i];
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    fake_i2c_write_calls++;
    fake_last_handle = i2c;
    fake_last_addr = dev_addr;
    fake_last_reg = reg_addr;
    fake_last_len = len;
    fake_last_timeout = timeout;

    for (size_t i = 0; i < len && i < FAKE_TX_MAX; i++) {
        fake_tx[i] = data[i];
    }
    return fake_i2c_write_status;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len, uint32_t timeout)
{
    int slot = fake_spi_tx_calls;

    fake_spi_tx_calls++;
    fake_last_handle = spi;
    fake_last_len = len;
    fake_last_timeout = timeout;

    if (slot < FAKE_TX_MAX) {
        fake_tx_count[slot] = len > 0 ? data[0] : 0;
        fake_tx_lens[slot] = len;
    }
    return fake_spi_tx_status;
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len, uint32_t timeout)
{
    fake_spi_rx_calls++;
    fake_last_handle = spi;
    fake_last_len = len;
    fake_last_timeout = timeout;

    if (fake_spi_rx_status != XY_HAL_OK) {
        return fake_spi_rx_status;
    }

    for (size_t i = 0; i < len && i < sizeof(fake_read_bytes); i++) {
        data[i] = fake_read_bytes[i];
    }
    return XY_HAL_OK;
}

/* ==================== Helpers ==================== */

static void bus_i2c(xy_sensor_bus_t *bus, uint8_t address)
{
    xy_sensor_bus_config_i2c(bus, (void *)0x1U, address);
}

static void bus_spi(xy_sensor_bus_t *bus)
{
    xy_sensor_bus_config_spi(bus, (void *)0x2U, 7U);
}

static void assert_hal_untouched(void)
{
    TEST_ASSERT_EQUAL_INT(0, fake_i2c_read_calls);
    TEST_ASSERT_EQUAL_INT(0, fake_i2c_write_calls);
    TEST_ASSERT_EQUAL_INT(0, fake_spi_tx_calls);
    TEST_ASSERT_EQUAL_INT(0, fake_spi_rx_calls);
}

void setUp(void)
{
    fake_reset();
}

void tearDown(void)
{
}

/* ==================== I2C rejection paths ==================== */

static void test_i2c_read_rejects_invalid_arguments_without_bus_traffic(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[2] = {0};

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(NULL, 0x01, data, 1));
    assert_hal_untouched();

    bus_i2c(&bus, 0x48);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, NULL, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 0));
    assert_hal_untouched();
}

static void test_i2c_read_rejects_unconfigured_bus(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[2] = {0};

    memset(&bus, 0, sizeof(bus));

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));
    assert_hal_untouched();

    /* A bus configured for SPI is not usable as an I2C bus. */
    bus_spi(&bus);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));
    assert_hal_untouched();
}

static void test_i2c_read_rejects_missing_handle_and_bad_address(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[2] = {0};

    xy_sensor_bus_config_i2c(&bus, NULL, 0x48);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    bus_i2c(&bus, 0x00);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    /* 0xA0 is 0x50 shifted left; the HAL takes unshifted addresses. */
    bus_i2c(&bus, 0xA0);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    bus_i2c(&bus, 0x80);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    assert_hal_untouched();
}

/* ==================== I2C transfer paths ==================== */

static void test_i2c_read_forwards_address_register_length_and_timeout(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[3] = {0};

    bus_i2c(&bus, 0x50);
    fake_read_bytes[0] = 0xAA;
    fake_read_bytes[1] = 0xBB;
    fake_read_bytes[2] = 0xCC;

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_i2c_read(&bus, 0x0D, data, sizeof(data)));

    TEST_ASSERT_EQUAL_INT(1, fake_i2c_read_calls);
    TEST_ASSERT_EQUAL_PTR((void *)0x1U, fake_last_handle);
    TEST_ASSERT_EQUAL_UINT16(0x50, fake_last_addr);
    TEST_ASSERT_EQUAL_UINT16(0x0D, fake_last_reg);
    TEST_ASSERT_EQUAL_UINT(3, fake_last_len);
    TEST_ASSERT_EQUAL_UINT32(100, fake_last_timeout);
    TEST_ASSERT_EQUAL_HEX8(0xAA, data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, data[2]);
}

static void test_i2c_read_maps_every_hal_failure_to_minus_one(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[1] = {0};

    bus_i2c(&bus, 0x50);

    fake_i2c_read_status = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    fake_i2c_read_status = XY_HAL_ERROR_NOT_SUPPORTED;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    fake_i2c_read_status = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read(&bus, 0x01, data, 1));

    TEST_ASSERT_EQUAL_INT(3, fake_i2c_read_calls);
}

static void test_i2c_write_forwards_payload_and_maps_failure(void)
{
    xy_sensor_bus_t bus;
    const uint8_t payload[2] = {0x12, 0x34};

    bus_i2c(&bus, 0x50);

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_i2c_write(&bus, 0x20, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(1, fake_i2c_write_calls);
    TEST_ASSERT_EQUAL_UINT16(0x20, fake_last_reg);
    TEST_ASSERT_EQUAL_UINT(2, fake_last_len);
    TEST_ASSERT_EQUAL_UINT32(100, fake_last_timeout);
    TEST_ASSERT_EQUAL_HEX8(0x12, fake_tx[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, fake_tx[1]);

    fake_i2c_write_status = XY_HAL_ERROR_BUSY;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_write(&bus, 0x20, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(2, fake_i2c_write_calls);
}

static void test_i2c_write_rejects_invalid_arguments_without_bus_traffic(void)
{
    xy_sensor_bus_t bus;
    const uint8_t payload[1] = {0x01};

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_write(NULL, 0x01, payload, 1));

    bus_i2c(&bus, 0x50);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_write(&bus, 0x01, NULL, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_write(&bus, 0x01, payload, 0));

    assert_hal_untouched();
}

static void test_i2c_single_byte_helpers_use_one_byte_transfers(void)
{
    xy_sensor_bus_t bus;
    uint8_t value = 0;
    const uint8_t payload[1] = {0x5A};

    bus_i2c(&bus, 0x50);
    fake_read_bytes[0] = 0x3C;

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_i2c_read_reg(&bus, 0x0F, &value));
    TEST_ASSERT_EQUAL_UINT(1, fake_last_len);
    TEST_ASSERT_EQUAL_HEX8(0x3C, value);

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_i2c_write_reg(&bus, 0x0F, 0x5A));
    TEST_ASSERT_EQUAL_UINT(1, fake_last_len);
    TEST_ASSERT_EQUAL_HEX8(payload[0], fake_tx[0]);
}

static void test_i2c_reg16_is_little_endian_like_smbus(void)
{
    xy_sensor_bus_t bus;
    uint16_t value = 0;

    bus_i2c(&bus, 0x0B);
    fake_read_bytes[0] = 0x34; /* least significant byte first */
    fake_read_bytes[1] = 0x12;

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_i2c_read_reg16(&bus, 0x09, &value));
    TEST_ASSERT_EQUAL_UINT(2, fake_last_len);
    TEST_ASSERT_EQUAL_HEX16(0x1234, value);

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_i2c_write_reg16(&bus, 0x09, 0x1234));
    TEST_ASSERT_EQUAL_UINT(2, fake_last_len);
    TEST_ASSERT_EQUAL_HEX8(0x34, fake_tx[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, fake_tx[1]);
}

static void test_i2c_reg16_guards_output_pointer_and_preserves_it_on_failure(void)
{
    xy_sensor_bus_t bus;
    uint16_t value = 0xBEEF;

    bus_i2c(&bus, 0x0B);

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read_reg16(&bus, 0x09, NULL));
    assert_hal_untouched();

    fake_i2c_read_status = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_i2c_read_reg16(&bus, 0x09, &value));
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, value);
}

/* ==================== SPI transfer paths ==================== */

static void test_spi_read_sends_register_then_receives(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[2] = {0};

    bus_spi(&bus);
    fake_read_bytes[0] = 0x11;
    fake_read_bytes[1] = 0x22;

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_spi_read(&bus, 0x80, data, sizeof(data)));

    TEST_ASSERT_EQUAL_INT(1, fake_spi_tx_calls);
    TEST_ASSERT_EQUAL_INT(1, fake_spi_rx_calls);
    TEST_ASSERT_EQUAL_HEX8(0x80, fake_tx_count[0]);
    TEST_ASSERT_EQUAL_UINT(1, fake_tx_lens[0]);
    TEST_ASSERT_EQUAL_UINT(2, fake_last_len);
    TEST_ASSERT_EQUAL_HEX8(0x11, data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22, data[1]);
}

static void test_spi_read_stops_when_the_register_write_fails(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[2] = {0};

    bus_spi(&bus);
    fake_spi_tx_status = XY_HAL_ERROR_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_read(&bus, 0x80, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(1, fake_spi_tx_calls);
    TEST_ASSERT_EQUAL_INT(0, fake_spi_rx_calls);
}

static void test_spi_read_propagates_receive_failure(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[2] = {0};

    bus_spi(&bus);
    fake_spi_rx_status = XY_HAL_ERROR_IO;

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_read(&bus, 0x80, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(1, fake_spi_rx_calls);
}

static void test_spi_write_sends_register_then_payload(void)
{
    xy_sensor_bus_t bus;
    const uint8_t payload[2] = {0xCA, 0xFE};

    bus_spi(&bus);

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_spi_write(&bus, 0x02, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(2, fake_spi_tx_calls);
    TEST_ASSERT_EQUAL_HEX8(0x02, fake_tx_count[0]);
    TEST_ASSERT_EQUAL_UINT(1, fake_tx_lens[0]);
    TEST_ASSERT_EQUAL_UINT(2, fake_tx_lens[1]);

    fake_spi_tx_status = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_write(&bus, 0x02, payload, sizeof(payload)));
}

static void test_spi_rejects_invalid_arguments_and_wrong_bus_type(void)
{
    xy_sensor_bus_t bus;
    uint8_t data[1] = {0};

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_read(NULL, 0x00, data, 1));

    bus_spi(&bus);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_read(&bus, 0x00, NULL, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_read(&bus, 0x00, data, 0));
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_write(&bus, 0x00, NULL, 1));

    assert_hal_untouched();

    bus_i2c(&bus, 0x50);
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_read(&bus, 0x00, data, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_spi_write(&bus, 0x00, data, 1));
    assert_hal_untouched();
}

/* ==================== Device identity helper ==================== */

static void test_check_device_id_accepts_only_the_expected_value(void)
{
    xy_sensor_bus_t bus;

    bus_i2c(&bus, 0x76);
    fake_read_bytes[0] = 0x58;

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_check_device_id(&bus, 0xD0, 0x58));
    TEST_ASSERT_EQUAL_INT(1, fake_i2c_read_calls);

    fake_read_bytes[0] = 0x59;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_check_device_id(&bus, 0xD0, 0x58));
}

static void test_check_device_id_fails_closed_on_bad_input(void)
{
    xy_sensor_bus_t bus;

    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_check_device_id(NULL, 0xD0, 0x58));

    /* A bus that was never configured has type 0, which is neither I2C nor SPI. */
    memset(&bus, 0, sizeof(bus));
    bus.bus_handle = (void *)0x1U;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_check_device_id(&bus, 0xD0, 0x58));
    assert_hal_untouched();

    bus_i2c(&bus, 0x76);
    fake_i2c_read_status = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(-1, xy_sensor_check_device_id(&bus, 0xD0, 0x58));
}

static void test_check_device_id_uses_the_spi_path_for_spi_buses(void)
{
    xy_sensor_bus_t bus;

    bus_spi(&bus);
    fake_read_bytes[0] = 0xAB;

    TEST_ASSERT_EQUAL_INT(0, xy_sensor_check_device_id(&bus, 0x0F, 0xAB));
    TEST_ASSERT_EQUAL_INT(0, fake_i2c_read_calls);
    TEST_ASSERT_EQUAL_INT(1, fake_spi_rx_calls);
}

/* ==================== Bus configuration ==================== */

static void test_bus_config_helpers_set_every_field(void)
{
    xy_sensor_bus_t bus;

    memset(&bus, 0xFF, sizeof(bus));
    xy_sensor_bus_config_i2c(&bus, (void *)0x3U, 0x29);
    TEST_ASSERT_EQUAL_INT(XY_SENSOR_BUS_I2C, bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x3U, bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(0x29, bus.address);
    TEST_ASSERT_EQUAL_UINT8(0, bus.chip_select);

    xy_sensor_bus_config_spi(&bus, (void *)0x4U, 12U);
    TEST_ASSERT_EQUAL_INT(XY_SENSOR_BUS_SPI, bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x4U, bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(0, bus.address);
    TEST_ASSERT_EQUAL_UINT8(12, bus.chip_select);
}

static void test_bus_config_helpers_ignore_a_null_bus(void)
{
    xy_sensor_bus_config_i2c(NULL, (void *)0x3U, 0x29);
    xy_sensor_bus_config_spi(NULL, (void *)0x3U, 1U);
    TEST_PASS();
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_i2c_read_rejects_invalid_arguments_without_bus_traffic);
    RUN_TEST(test_i2c_read_rejects_unconfigured_bus);
    RUN_TEST(test_i2c_read_rejects_missing_handle_and_bad_address);
    RUN_TEST(test_i2c_read_forwards_address_register_length_and_timeout);
    RUN_TEST(test_i2c_read_maps_every_hal_failure_to_minus_one);
    RUN_TEST(test_i2c_write_forwards_payload_and_maps_failure);
    RUN_TEST(test_i2c_write_rejects_invalid_arguments_without_bus_traffic);
    RUN_TEST(test_i2c_single_byte_helpers_use_one_byte_transfers);
    RUN_TEST(test_i2c_reg16_is_little_endian_like_smbus);
    RUN_TEST(test_i2c_reg16_guards_output_pointer_and_preserves_it_on_failure);
    RUN_TEST(test_spi_read_sends_register_then_receives);
    RUN_TEST(test_spi_read_stops_when_the_register_write_fails);
    RUN_TEST(test_spi_read_propagates_receive_failure);
    RUN_TEST(test_spi_write_sends_register_then_payload);
    RUN_TEST(test_spi_rejects_invalid_arguments_and_wrong_bus_type);
    RUN_TEST(test_check_device_id_accepts_only_the_expected_value);
    RUN_TEST(test_check_device_id_fails_closed_on_bad_input);
    RUN_TEST(test_check_device_id_uses_the_spi_path_for_spi_buses);
    RUN_TEST(test_bus_config_helpers_set_every_field);
    RUN_TEST(test_bus_config_helpers_ignore_a_null_bus);

    return UNITY_END();
}
