#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_bmp390.h"
#include "xy_hal_delay.h"

static uint8_t g_regs[256];
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint32_t g_delay_ms;
static uint32_t g_fail_read_at;
static uint32_t g_fail_write_at;
static xy_error_t g_i2c_init_result;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr,
                              uint32_t timeout)
{
    if (g_i2c_init_result != XY_DEVICE_OK) {
        return g_i2c_init_result;
    }
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    g_read_count++;
    if (g_fail_read_at == g_read_count) {
        return XY_DEVICE_TIMEOUT;
    }
    memcpy(data, &g_regs[reg], len);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    g_write_count++;
    if (g_fail_write_at == g_write_count) {
        return XY_DEVICE_BUSY;
    }
    memcpy(&g_regs[reg], data, len);
    if (reg == BMP3_REG_CMD && len == 1U && data[0] == BMP3_SOFT_RESET) {
        g_regs[BMP3_REG_SENS_STATUS] = BMP3_CMD_RDY;
        g_regs[BMP3_REG_ERR] = 0U;
    }
    return XY_DEVICE_OK;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

static void load_fixture(void)
{
    static const uint8_t calibration[BMP3_LEN_CALIB_DATA] = {
        0x70, 0x6B, 0x43, 0x67, 0xF7, 0xD0, 0x3B, 0x00, 0x2D, 0xD6, 0xD0,
        0x0B, 0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF, 0x0C, 0x30, 0x00,
    };
    static const uint8_t sample[BMP3_LEN_P_T_DATA] = {0x00, 0x00, 0x80, 0x00, 0x00, 0x80};

    memset(g_regs, 0, sizeof(g_regs));
    g_regs[BMP3_REG_CHIP_ID] = BMP390_CHIP_ID;
    g_regs[BMP3_REG_SENS_STATUS] = BMP3_CMD_RDY;
    memcpy(&g_regs[BMP3_REG_CALIB_DATA], calibration, sizeof(calibration));
    memcpy(&g_regs[BMP3_REG_DATA], sample, sizeof(sample));
}

void setUp(void)
{
    load_fixture();
    g_read_count = 0U;
    g_write_count = 0U;
    g_delay_ms = 0U;
    g_fail_read_at = 0U;
    g_fail_write_at = 0U;
    g_i2c_init_result = XY_DEVICE_OK;
}

void tearDown(void)
{
}

static void test_init_reads_identity_calibration_and_configures_normal_mode(void)
{
    xy_bmp390_t dev;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bmp390_init(NULL, &bus, XY_BMP390_ADDR_PRIMARY));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bmp390_init(&dev, NULL, XY_BMP390_ADDR_PRIMARY));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp390_init(&dev, &bus, 0x70U));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bmp390_init(&dev, &bus, XY_BMP390_ADDR_SECONDARY));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(BMP390_CHIP_ID, dev.bosch.chip_id);
    TEST_ASSERT_EQUAL_UINT16(XY_BMP390_ADDR_SECONDARY, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT8(BMP3_ENABLE, dev.settings.press_en);
    TEST_ASSERT_EQUAL_UINT8(BMP3_ENABLE, dev.settings.temp_en);
    TEST_ASSERT_EQUAL_UINT8(BMP3_MODE_NORMAL, dev.settings.op_mode);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, g_read_count);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, g_write_count);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, g_delay_ms);
}

static void test_read_uses_bosch_compensation_and_commits_atomically(void)
{
    xy_bmp390_t dev;
    xy_bmp390_data_t output = {0};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bmp390_init(&dev, &bus, XY_BMP390_ADDR_PRIMARY));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp390_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT64(3311, output.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT64(8844420ULL, output.pressure_centi_pa);
    TEST_ASSERT_EQUAL_MEMORY(&output, &dev.data, sizeof(output));

    output.temperature_centi_c = 1234;
    output.pressure_centi_pa = 5678U;
    dev.data.temperature_centi_c = -111;
    dev.data.pressure_centi_pa = 222U;
    const xy_bmp390_data_t output_before = output;
    const xy_bmp390_data_t cache_before = dev.data;
    g_fail_read_at = g_read_count + 1U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_bmp390_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_before, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_before, &dev.data, sizeof(dev.data));
}

static void test_failures_clear_or_preserve_lifecycle(void)
{
    xy_bmp390_t dev;
    xy_bmp390_data_t output = {.temperature_centi_c = 12, .pressure_centi_pa = 34U};
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_i2c_init_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_bmp390_init(&dev, &bus, XY_BMP390_ADDR_PRIMARY));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);

    setUp();
    g_fail_read_at = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_bmp390_init(&dev, &bus, XY_BMP390_ADDR_PRIMARY));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);

    setUp();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bmp390_init(&dev, &bus, XY_BMP390_ADDR_PRIMARY));
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp390_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT32(12, output.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT64(34U, output.pressure_centi_pa);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp390_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);

    dev.i2c_dev.base.initialized = 1U;
    const uint8_t mode_before = dev.settings.op_mode;
    g_fail_write_at = g_write_count + 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_bmp390_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(mode_before, dev.settings.op_mode);
}

static void test_public_operations_require_live_nested_handle_and_deinit_clears_it(void)
{
    xy_bmp390_t dev;
    xy_bmp390_data_t output = {.temperature_centi_c = 12, .pressure_centi_pa = 34U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bmp390_init(&dev, &bus, XY_BMP390_ADDR_PRIMARY));
    dev.i2c_dev.i2c_handle = NULL;
    const uint32_t reads_before = g_read_count;
    const uint32_t writes_before = g_write_count;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp390_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp390_deinit(&dev));
    TEST_ASSERT_EQUAL_INT32(12, output.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT64(34U, output.pressure_centi_pa);
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
    TEST_ASSERT_TRUE(dev.initialized);

    dev.i2c_dev.i2c_handle = &bus;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp390_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_reads_identity_calibration_and_configures_normal_mode);
    RUN_TEST(test_read_uses_bosch_compensation_and_commits_atomically);
    RUN_TEST(test_failures_clear_or_preserve_lifecycle);
    RUN_TEST(test_public_operations_require_live_nested_handle_and_deinit_clears_it);
    return UNITY_END();
}
