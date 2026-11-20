#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_icm20608.h"

/*
 * Focused canonical Device-driver test for xy_icm20608 (I2C + SPI).
 *
 * The legacy `sensor_icm20608.c` wrapper delegated init/deinit/read/range/odr/
 * dlpf/bias/configuration/interrupt behavior to this driver and mapped the
 * owner errors onto legacy `sensor_err_t` codes. This test asserts those
 * contracts on the canonical API directly so the wrapper can be retired
 * without losing coverage: identity/reset/configuration, both transports,
 * sample/accel/gyro/temperature decode, range/odr/dlpf/bias, sleep, the
 * data-ready interrupt, configuration verification, and atomic failure.
 *
 * The fake I2C/SPI transport is a register file: writes store the programmed
 * value, reads return the stored value, so the driver's own read-back
 * verification and update_bits read-modify-write are exercised against real
 * state. One-shot transport-error and per-register read-override injections
 * cover failure and value-mismatch paths.
 */

static uint8_t g_regs[256];
static xy_error_t g_next_read_error;
static int g_read_error_pending;
static xy_error_t g_next_write_error;
static int g_write_error_pending;
static uint8_t g_override_reg;
static uint8_t g_override_value;
static int g_override_active;
static uint32_t g_delay_ms;

static void fake_bus_reset(void)
{
    memset(g_regs, 0, sizeof(g_regs));
    g_next_read_error = XY_DEVICE_OK;
    g_read_error_pending = 0;
    g_next_write_error = XY_DEVICE_OK;
    g_write_error_pending = 0;
    g_override_reg = 0U;
    g_override_value = 0U;
    g_override_active = 0;
    g_delay_ms = 0U;
}

static void inject_read_error(xy_error_t error)
{
    g_next_read_error = error;
    g_read_error_pending = 1;
}

static void inject_write_error(xy_error_t error)
{
    g_next_write_error = error;
    g_write_error_pending = 1;
}

static void override_read(uint8_t reg, uint8_t value)
{
    g_override_reg = reg;
    g_override_value = value;
    g_override_active = 1;
}

static void fake_bus_read(uint8_t reg, uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (g_read_error_pending) {
        g_read_error_pending = 0;
        return; /* caller handles the injected error itself */
    }
    for (i = 0U; i < len; ++i) {
        uint8_t addr = (uint8_t)(reg + i);
        data[i] = (g_override_active && addr == g_override_reg) ? g_override_value
                                                                : g_regs[addr];
    }
}

static xy_error_t fake_bus_read_result(void)
{
    if (g_read_error_pending) {
        g_read_error_pending = 0;
        return g_next_read_error;
    }
    return XY_DEVICE_OK;
}

static xy_error_t fake_bus_write(uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (g_write_error_pending) {
        g_write_error_pending = 0;
        return g_next_write_error;
    }
    for (i = 0U; i < len; ++i) {
        g_regs[(uint8_t)(reg + i)] = data[i];
    }
    return XY_DEVICE_OK;
}

/* --- Device I2C helper stubs (the driver's transport seam) --- */

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_TRUE(address == XY_ICM20608_ADDR_DEFAULT || address == XY_ICM20608_ADDR_ALT);
    TEST_ASSERT_EQUAL_UINT32(100U, timeout);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    xy_error_t result = fake_bus_read_result();

    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    fake_bus_read(reg, data, (uint16_t)len);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    return fake_bus_write(reg, data, (uint16_t)len);
}

/* --- SPI callbacks (same register file) --- */

static void *g_spi_context;

static xy_error_t icm_spi_read(void *context, uint8_t reg, uint8_t *data, uint16_t len)
{
    xy_error_t result;

    TEST_ASSERT_EQUAL_PTR(g_spi_context, context);
    result = fake_bus_read_result();
    if (result != XY_DEVICE_OK) {
        return result;
    }
    fake_bus_read(reg, data, len);
    return XY_DEVICE_OK;
}

static xy_error_t icm_spi_write(void *context, uint8_t reg, const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_EQUAL_PTR(g_spi_context, context);
    return fake_bus_write(reg, data, len);
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

/* --- init helpers --- */

static void init_i2c(xy_icm20608_t *dev, int *bus)
{
    g_regs[XY_ICM20608_REG_WHO_AM_I] = XY_ICM20608_WHO_AM_I;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_init_i2c(dev, bus, XY_ICM20608_ADDR_DEFAULT));
}

static void init_spi(xy_icm20608_t *dev)
{
    g_spi_context = dev;
    g_regs[XY_ICM20608_REG_WHO_AM_I] = XY_ICM20608_WHO_AM_I;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_init_spi(dev, dev, icm_spi_read, icm_spi_write));
}

void setUp(void)
{
    fake_bus_reset();
    g_spi_context = NULL;
}

void tearDown(void)
{
}

/* --- identity / init --- */

static void test_init_i2c_guards_and_identity(void)
{
    xy_icm20608_t dev;
    xy_icm20608_t before;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    before = dev;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_init_i2c(NULL, &bus, 0x68U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_init_i2c(&dev, NULL, 0x68U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_init_i2c(&dev, &bus, 0x67U));
    TEST_ASSERT_EQUAL_MEMORY(&before, &dev, sizeof(dev));

    g_regs[XY_ICM20608_REG_WHO_AM_I] = 0xAFU; /* wrong identity */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,
                          xy_icm20608_init_i2c(&dev, &bus, XY_ICM20608_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_MEMORY(&before, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_ms);
}

static void test_init_i2c_configures_and_verifies(void)
{
    static const uint8_t expected_regs[] = {
        XY_ICM20608_REG_PWR_MGMT_1,  XY_ICM20608_REG_PWR_MGMT_2,
        XY_ICM20608_REG_GYRO_CONFIG, XY_ICM20608_REG_ACCEL_CONFIG,
        XY_ICM20608_REG_SMPLRT_DIV,  XY_ICM20608_REG_CONFIG,
        XY_ICM20608_REG_ACCEL_CONFIG2,
    };
    static const uint8_t expected[] = {0x01U, 0x00U, 0x08U, 0x08U, 0x09U, 0x04U, 0x04U};
    xy_icm20608_t dev;
    int bus;
    size_t i;

    init_i2c(&dev, &bus);
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.configuration_synchronized);
    TEST_ASSERT_TRUE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_PTR(&bus, dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_TRANSPORT_I2C, dev.transport);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, dev.accel_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_500DPS, dev.gyro_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_20HZ, dev.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_21HZ, dev.accel_dlpf);
    TEST_ASSERT_EQUAL_UINT16(100U, dev.odr_hz);
    TEST_ASSERT_EQUAL_UINT32(100U, g_delay_ms);
    for (i = 0U; i < sizeof(expected); ++i) {
        TEST_ASSERT_EQUAL_UINT8(expected[i], g_regs[expected_regs[i]]);
    }
}

static void test_init_i2c_reset_write_failure_is_atomic(void)
{
    xy_icm20608_t dev;
    xy_icm20608_t before;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    before = dev;
    g_regs[XY_ICM20608_REG_WHO_AM_I] = XY_ICM20608_WHO_AM_I;
    inject_write_error(XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_icm20608_init_i2c(&dev, &bus, XY_ICM20608_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_MEMORY(&before, &dev, sizeof(dev));
}

static void test_init_i2c_rejects_config_readback_mismatch(void)
{
    xy_icm20608_t dev;
    xy_icm20608_t before;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    before = dev;
    g_regs[XY_ICM20608_REG_WHO_AM_I] = XY_ICM20608_WHO_AM_I;
    override_read(XY_ICM20608_REG_PWR_MGMT_1, 0x00U); /* read-back does not stick */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL,
                          xy_icm20608_init_i2c(&dev, &bus, XY_ICM20608_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_MEMORY(&before, &dev, sizeof(dev));
}

static void test_init_spi_guards_and_path(void)
{
    xy_icm20608_t dev;
    int ctx;

    memset(&dev, 0xA5, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_init_spi(NULL, &ctx, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_init_spi(&dev, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_init_spi(&dev, &ctx, NULL, icm_spi_write));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_init_spi(&dev, &ctx, icm_spi_read, NULL));

    g_spi_context = &dev;
    init_spi(&dev);
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_TRANSPORT_SPI, dev.transport);
    TEST_ASSERT_EQUAL_PTR(&dev, dev.spi_context);
}

/* --- decode --- */

static void test_read_sample_decodes_all_axes_and_temperature(void)
{
    static const uint8_t raw[14] = {
        0x20, 0x00, 0xE0, 0x00, 0x40, 0x00, 0x01,
        0x46, 0x10, 0x00, 0xF0, 0x00, 0x08, 0x00,
    };
    xy_icm20608_sample_t sample = {0};
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    memcpy(&g_regs[XY_ICM20608_REG_ACCEL_XOUT_H], raw, sizeof(raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&dev, &sample));
    TEST_ASSERT_EQUAL_INT32(1000, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-1000, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(2000, sample.accel.z_mg);
    TEST_ASSERT_EQUAL_INT32(2599, sample.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(62500, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-62500, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(31250, sample.gyro.z_mdps);
    TEST_ASSERT_EQUAL_MEMORY(&sample.accel, &dev.accel, sizeof(sample.accel));
    TEST_ASSERT_EQUAL_MEMORY(&sample.gyro, &dev.gyro, sizeof(sample.gyro));
}

static void test_read_accel_gyro_temperature(void)
{
    static const uint8_t accel_raw[6] = {0x00, 0x10, 0xFF, 0xF0, 0x20, 0x00};
    static const uint8_t gyro_raw[6] = {0x00, 0x40, 0xFF, 0xC0, 0x10, 0x00};
    static const uint8_t temp_raw[2] = {0x01, 0x46};
    xy_icm20608_accel_t accel;
    xy_icm20608_gyro_t gyro;
    int32_t temperature = 0;
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);

    memcpy(&g_regs[XY_ICM20608_REG_ACCEL_XOUT_H], accel_raw, sizeof(accel_raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_accel(&dev, &accel));
    TEST_ASSERT_EQUAL_INT32(1, accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-1, accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(1000, accel.z_mg);

    memcpy(&g_regs[XY_ICM20608_REG_GYRO_XOUT_H], gyro_raw, sizeof(gyro_raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_gyro(&dev, &gyro));
    TEST_ASSERT_EQUAL_INT32(976, gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-976, gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(62500, gyro.z_mdps);

    memcpy(&g_regs[XY_ICM20608_REG_TEMP_OUT_H], temp_raw, sizeof(temp_raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT32(2599, temperature);
}

static void test_read_guards_and_failure_preserve_output(void)
{
    xy_icm20608_accel_t accel = {11, 22, 33};
    xy_icm20608_accel_t snapshot = accel;
    xy_icm20608_sample_t sample = {0};
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_read_accel(NULL, &accel));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_read_accel(&dev, NULL));

    inject_read_error(XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_icm20608_read_accel(&dev, &accel));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &accel, sizeof(accel));

    dev.sleeping = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_read_sample(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_read_accel(&dev, &accel));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &accel, sizeof(accel));
}

/* --- configuration --- */

static void test_set_range_updates_register_and_cache(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_accel_range(&dev, XY_ICM20608_ACCEL_RANGE_16G));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_16G, dev.accel_range);
    TEST_ASSERT_EQUAL_UINT8(0x18U, g_regs[XY_ICM20608_REG_ACCEL_CONFIG] & 0x18U);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_gyro_range(&dev, XY_ICM20608_GYRO_RANGE_2000DPS));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_2000DPS, dev.gyro_range);
    TEST_ASSERT_EQUAL_UINT8(0x18U, g_regs[XY_ICM20608_REG_GYRO_CONFIG] & 0x18U);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_accel_range(&dev, (xy_icm20608_accel_range_t)4));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_gyro_range(&dev, (xy_icm20608_gyro_range_t)4));
}

static void test_set_range_write_mismatch_fail_closes(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    override_read(XY_ICM20608_REG_ACCEL_CONFIG, 0x00U); /* programmed value not seen */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL,
                          xy_icm20608_set_accel_range(&dev, XY_ICM20608_ACCEL_RANGE_16G));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, dev.accel_range);
    TEST_ASSERT_FALSE(dev.configuration_synchronized);
}

static void test_set_range_rejects_incompatible_bias(void)
{
    const xy_icm20608_accel_t accel_bias = {3000, 0, 0};
    const xy_icm20608_gyro_t gyro_bias = {0, 0, 0};
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_bias(&dev, &accel_bias, &gyro_bias));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_accel_range(&dev, XY_ICM20608_ACCEL_RANGE_2G));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, dev.accel_range);
}

static void test_set_odr_is_exact_and_failure_atomic(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_odr(&dev, 100U));
    TEST_ASSERT_EQUAL_UINT16(100U, dev.odr_hz);
    TEST_ASSERT_EQUAL_UINT8(0x09U, g_regs[XY_ICM20608_REG_SMPLRT_DIV]);

    inject_write_error(XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_icm20608_set_odr(&dev, 200U));
    TEST_ASSERT_EQUAL_UINT16(100U, dev.odr_hz);

    override_read(XY_ICM20608_REG_SMPLRT_DIV, 0x09U); /* stale divider after write */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL, xy_icm20608_set_odr(&dev, 200U));
    TEST_ASSERT_EQUAL_UINT16(100U, dev.odr_hz);
    TEST_ASSERT_FALSE(dev.configuration_synchronized);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_set_odr(&dev, 0U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_set_odr(&dev, 333U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_set_odr(&dev, 1001U));
}

static void test_set_dlpf_preserves_bits_and_rolls_back(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_dlpf(&dev, XY_ICM20608_DLPF_92HZ,
                                               XY_ICM20608_ACCEL_DLPF_218HZ_ALT));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, dev.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_218HZ_ALT, dev.accel_dlpf);

    /* accel-config write fails: the driver must roll the gyro config back. */
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, dev.gyro_dlpf);
    inject_write_error(XY_DEVICE_IO_ERROR); /* fails the CONFIG write in the next attempt */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_icm20608_set_dlpf(&dev, XY_ICM20608_DLPF_10HZ,
                                               XY_ICM20608_ACCEL_DLPF_10HZ));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, dev.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_218HZ_ALT, dev.accel_dlpf);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_dlpf(&dev, (xy_icm20608_dlpf_t)7,
                                               XY_ICM20608_ACCEL_DLPF_21HZ));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_dlpf(&dev, XY_ICM20608_DLPF_20HZ,
                                               (xy_icm20608_accel_dlpf_t)7));
}

static void test_set_and_get_bias(void)
{
    const xy_icm20608_accel_t accel_bias = {50, -25, 100};
    const xy_icm20608_gyro_t gyro_bias = {10000, -5000, 20000};
    xy_icm20608_accel_t accel_out = {1, 2, 3};
    xy_icm20608_gyro_t gyro_out = {4, 5, 6};
    xy_icm20608_accel_t invalid = accel_bias;
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_bias(&dev, &accel_bias, &gyro_bias));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &dev.accel_bias, sizeof(accel_bias));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &dev.gyro_bias, sizeof(gyro_bias));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_get_bias(&dev, &accel_out, &gyro_out));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &accel_out, sizeof(accel_out));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &gyro_out, sizeof(gyro_out));

    invalid.x_mg = 4001; /* exceeds 4G full-scale in mg */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_bias(&dev, &invalid, &gyro_bias));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &dev.accel_bias, sizeof(accel_bias));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_set_bias(&dev, NULL, &gyro_bias));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_set_bias(&dev, &accel_bias, NULL));
}

static void test_bias_applied_to_sample_paths(void)
{
    static const uint8_t sample_raw[14] = {
        0x08, 0x00, 0xF8, 0x00, 0x20, 0x00, 0x00,
        0x00, 0x04, 0x00, 0xFC, 0x00, 0x10, 0x00,
    };
    const xy_icm20608_accel_t accel_bias = {50, -25, 100};
    const xy_icm20608_gyro_t gyro_bias = {10000, -5000, 20000};
    xy_icm20608_sample_t sample;
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_bias(&dev, &accel_bias, &gyro_bias));
    memcpy(&g_regs[XY_ICM20608_REG_ACCEL_XOUT_H], sample_raw, sizeof(sample_raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&dev, &sample));
    TEST_ASSERT_EQUAL_INT32(200, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-225, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(900, sample.accel.z_mg);
    TEST_ASSERT_EQUAL_INT32(5625, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-10625, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(42500, sample.gyro.z_mdps);
}

/* --- power / interrupt --- */

static void test_set_sleep_transitions_and_wake_delay(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_sleep(&dev, 1U));
    TEST_ASSERT_TRUE(dev.sleeping);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_sleep(&dev, 0U));
    TEST_ASSERT_FALSE(dev.sleeping);
    TEST_ASSERT_EQUAL_UINT32(135U, g_delay_ms); /* 100 init + 35 wake */

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_set_sleep(&dev, 2U));
}

static void test_set_sleep_write_mismatch_preserves_cache(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    override_read(XY_ICM20608_REG_PWR_MGMT_1, 0x01U); /* sleep bit not seen */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL, xy_icm20608_set_sleep(&dev, 1U));
    TEST_ASSERT_FALSE(dev.sleeping);
    TEST_ASSERT_FALSE(dev.configuration_synchronized);
}

static void test_data_ready_interrupt_and_status(void)
{
    xy_icm20608_t dev;
    uint8_t status = 0xCCU;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_data_ready_interrupt(&dev, 1U));
    TEST_ASSERT_TRUE(dev.data_ready_interrupt_enabled);

    g_regs[XY_ICM20608_REG_INT_STATUS] = 0x11U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_interrupt_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT8(0x11U, status);

    inject_read_error(XY_DEVICE_TIMEOUT);
    status = 0xCCU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_icm20608_read_interrupt_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT8(0xCCU, status);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_set_data_ready_interrupt(&dev, 0U));
    TEST_ASSERT_FALSE(dev.data_ready_interrupt_enabled);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_data_ready_interrupt(&dev, 2U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_read_interrupt_status(&dev, NULL));
}

/* --- configuration verification --- */

static void test_get_configuration_reflects_cached_state(void)
{
    xy_icm20608_configuration_t configuration = {
        .accel_range = XY_ICM20608_ACCEL_RANGE_16G,
        .gyro_range = XY_ICM20608_GYRO_RANGE_2000DPS,
        .gyro_dlpf = XY_ICM20608_DLPF_5HZ,
        .accel_dlpf = XY_ICM20608_ACCEL_DLPF_5HZ,
        .odr_hz = 999U,
        .sleeping = 1U,
        .data_ready_interrupt_enabled = 1U,
    };
    const xy_icm20608_configuration_t sentinel = configuration;
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_get_configuration(&dev, &configuration));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, configuration.accel_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_500DPS, configuration.gyro_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_20HZ, configuration.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_21HZ, configuration.accel_dlpf);
    TEST_ASSERT_EQUAL_UINT16(100U, configuration.odr_hz);
    TEST_ASSERT_FALSE(configuration.sleeping);
    TEST_ASSERT_FALSE(configuration.data_ready_interrupt_enabled);

    configuration = sentinel;
    dev.configuration_synchronized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_get_configuration(&dev, &configuration));
    TEST_ASSERT_EQUAL_MEMORY(&sentinel, &configuration, sizeof(configuration));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_get_configuration(&dev, NULL));
}

static void test_verify_configuration_detects_identity_drift(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    override_read(XY_ICM20608_REG_WHO_AM_I, 0xAFU);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL, xy_icm20608_verify_configuration(&dev));
    TEST_ASSERT_FALSE(dev.configuration_synchronized);
}

static void test_verify_configuration_propagates_transport_error(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    inject_read_error(XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_icm20608_verify_configuration(&dev));
    TEST_ASSERT_FALSE(dev.configuration_synchronized);
}

static void test_verify_configuration_passes_on_programmed_state(void)
{
    xy_icm20608_t dev;
    int bus;

    init_i2c(&dev, &bus);
    /* init already programmed and verified the registers; a fresh verify sees them. */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_verify_configuration(&dev));
    TEST_ASSERT_TRUE(dev.configuration_synchronized);
}

/* --- deinit --- */

static void test_deinit_clears_transport_and_fails_closed(void)
{
    xy_icm20608_t dev;
    xy_icm20608_t snapshot;
    int bus;

    init_i2c(&dev, &bus);
    snapshot = dev;
    inject_read_error(XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_icm20608_deinit(&dev));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_icm20608_deinit(&dev));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_i2c_guards_and_identity);
    RUN_TEST(test_init_i2c_configures_and_verifies);
    RUN_TEST(test_init_i2c_reset_write_failure_is_atomic);
    RUN_TEST(test_init_i2c_rejects_config_readback_mismatch);
    RUN_TEST(test_init_spi_guards_and_path);
    RUN_TEST(test_read_sample_decodes_all_axes_and_temperature);
    RUN_TEST(test_read_accel_gyro_temperature);
    RUN_TEST(test_read_guards_and_failure_preserve_output);
    RUN_TEST(test_set_range_updates_register_and_cache);
    RUN_TEST(test_set_range_write_mismatch_fail_closes);
    RUN_TEST(test_set_range_rejects_incompatible_bias);
    RUN_TEST(test_set_odr_is_exact_and_failure_atomic);
    RUN_TEST(test_set_dlpf_preserves_bits_and_rolls_back);
    RUN_TEST(test_set_and_get_bias);
    RUN_TEST(test_bias_applied_to_sample_paths);
    RUN_TEST(test_set_sleep_transitions_and_wake_delay);
    RUN_TEST(test_set_sleep_write_mismatch_preserves_cache);
    RUN_TEST(test_data_ready_interrupt_and_status);
    RUN_TEST(test_get_configuration_reflects_cached_state);
    RUN_TEST(test_verify_configuration_detects_identity_drift);
    RUN_TEST(test_verify_configuration_propagates_transport_error);
    RUN_TEST(test_verify_configuration_passes_on_programmed_state);
    RUN_TEST(test_deinit_clears_transport_and_fails_closed);
    return UNITY_END();
}
