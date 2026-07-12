#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_lps22hb.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    xy_ret_t ret;
} read_op_t;

typedef struct {
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    xy_ret_t ret;
} write_op_t;

static read_op_t g_reads[128];
static write_op_t g_writes[128];
static uint8_t g_seen_write_regs[128];
static uint8_t g_seen_write_values[128];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static size_t g_seen_write_count;
static uint32_t g_delay_total;
static size_t g_delay_count;

xy_ret_t xy_read_reg(xy_interface_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_index);
    TEST_ASSERT_EQUAL_UINT8(g_reads[g_read_index].reg, reg_addr);
    TEST_ASSERT_EQUAL_UINT16(g_reads[g_read_index].len, len);

    xy_ret_t ret = g_reads[g_read_index].ret;
    if (ret == XY_OK) {
        memcpy(data, g_reads[g_read_index].data, len);
    }
    g_read_index++;
    return ret;
}

xy_ret_t xy_write_reg(xy_interface_dev_t *dev, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_index);
    TEST_ASSERT_EQUAL_UINT8(g_writes[g_write_index].reg, reg_addr);
    TEST_ASSERT_EQUAL_UINT16(g_writes[g_write_index].len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(g_writes[g_write_index].data, data, len);

    g_seen_write_regs[g_seen_write_count] = reg_addr;
    g_seen_write_values[g_seen_write_count] = data[0];
    g_seen_write_count++;

    return g_writes[g_write_index++].ret;
}

void xy_delay_ms(uint32_t ms)
{
    g_delay_total += ms;
    g_delay_count++;
}

static void queue_read(uint8_t reg, const uint8_t *data, uint16_t len, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_reads[0].data), len);
    g_reads[g_read_count].reg = reg;
    g_reads[g_read_count].len = len;
    g_reads[g_read_count].ret = ret;
    if (data != NULL && len > 0U) {
        memcpy(g_reads[g_read_count].data, data, len);
    }
    g_read_count++;
}

static void queue_read8(uint8_t reg, uint8_t value, xy_ret_t ret)
{
    queue_read(reg, &value, 1U, ret);
}

static void queue_write(uint8_t reg, const uint8_t *data, uint16_t len, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_writes[0].data), len);
    g_writes[g_write_count].reg = reg;
    g_writes[g_write_count].len = len;
    g_writes[g_write_count].ret = ret;
    if (data != NULL && len > 0U) {
        memcpy(g_writes[g_write_count].data, data, len);
    }
    g_write_count++;
}

static void queue_write8(uint8_t reg, uint8_t value, xy_ret_t ret)
{
    queue_write(reg, &value, 1U, ret);
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    memset(g_seen_write_regs, 0, sizeof(g_seen_write_regs));
    memset(g_seen_write_values, 0, sizeof(g_seen_write_values));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_seen_write_count = 0;
    g_delay_total = 0;
    g_delay_count = 0;
}

void tearDown(void)
{
}

static xy_interface_dev_t fake_interface(void)
{
    xy_interface_dev_t iface = {.handle = (void *)0x1234, .address = LPS22HB_I2C_ADDR, .is_spi = false};
    return iface;
}

static void queue_init_success(void)
{
    queue_read8(LPS22HB_WHO_AM_I, LPS22HB_WHO_AM_I_VALUE, XY_OK);
    queue_read8(LPS22HB_CTRL_REG2, 0x00U, XY_OK);
    queue_write8(LPS22HB_CTRL_REG2, LPS22HB_SWRESET, XY_OK);
    queue_read8(LPS22HB_CTRL_REG2, 0x00U, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, (uint8_t)(XY_LPS22HB_ODR_10HZ | LPS22HB_EN_LPFP | XY_LPS22HB_LPF_ODR_20), XY_OK);
    queue_write8(LPS22HB_CTRL_REG2, 0x00U, XY_OK);
    queue_write8(LPS22HB_CTRL_REG3, 0x00U, XY_OK);
}

static void init_ok(xy_lps22hb_dev_t *dev, xy_interface_dev_t *iface)
{
    queue_init_success();
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_init(dev, iface, NULL));
}

static void test_init_default_config_resets_and_programs_registers(void)
{
    xy_lps22hb_dev_t dev;
    xy_interface_dev_t iface = fake_interface();

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_init(NULL, &iface, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_init(&dev, NULL, NULL));

    init_ok(&dev, &iface);
    TEST_ASSERT_TRUE(dev.is_initialized);
    TEST_ASSERT_EQUAL_UINT8(LPS22HB_WHO_AM_I_VALUE, dev.who_am_i);
    TEST_ASSERT_EQUAL_INT(XY_LPS22HB_ODR_10HZ, dev.config.odr);
    TEST_ASSERT_TRUE(dev.config.enable_lpf);
    TEST_ASSERT_FALSE(dev.config.enable_fifo);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1013.25f, dev.sea_level_pressure);
    TEST_ASSERT_EQUAL_UINT32(40U, g_delay_total);
    TEST_ASSERT_EQUAL_UINT(4U, g_seen_write_count);
}

static void test_init_rejects_bad_whoami_and_propagates_reset_timeout(void)
{
    xy_lps22hb_dev_t dev;
    xy_interface_dev_t iface = fake_interface();

    queue_read8(LPS22HB_WHO_AM_I, 0x00U, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_init(&dev, &iface, NULL));

    setUp();
    queue_read8(LPS22HB_WHO_AM_I, LPS22HB_WHO_AM_I_VALUE, XY_OK);
    queue_read8(LPS22HB_CTRL_REG2, 0x00U, XY_OK);
    queue_write8(LPS22HB_CTRL_REG2, LPS22HB_SWRESET, XY_OK);
    for (int i = 0; i < 100; i++) {
        queue_read8(LPS22HB_CTRL_REG2, LPS22HB_BOOT, XY_OK);
    }
    TEST_ASSERT_EQUAL_INT(-2, xy_lps22hb_init(&dev, &iface, NULL));
}

static void test_read_data_converts_pressure_temperature_and_offsets(void)
{
    xy_lps22hb_dev_t dev;
    xy_lps22hb_data_t data;
    xy_interface_dev_t iface = fake_interface();
    uint8_t sample[5] = {0x00U, 0x80U, 0x3EU, 0xC4U, 0x09U}; /* 1000.0 hPa, 25.0 C */

    init_ok(&dev, &iface);
    xy_lps22hb_set_pressure_offset(&dev, 1.5f);
    xy_lps22hb_set_temperature_offset(&dev, -0.25f);
    xy_lps22hb_set_sea_level_pressure(&dev, 1013.25f);

    queue_read(LPS22HB_PRESS_OUT_XL, sample, sizeof(sample), XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_read_data(&dev, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1001.5f, data.pressure);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 24.75f, data.temperature);
    TEST_ASSERT_TRUE(data.altitude > 90.0f);
    TEST_ASSERT_EQUAL_UINT32(1U, dev.measurement_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, data.pressure, dev.last_data.pressure);

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_read_data(NULL, &data));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_read_data(&dev, NULL));
}

static void test_measure_waits_for_data_ready_and_handles_timeout(void)
{
    xy_lps22hb_dev_t dev;
    xy_lps22hb_data_t data;
    xy_interface_dev_t iface = fake_interface();
    uint8_t sample[5] = {0x00U, 0x80U, 0x3EU, 0xC4U, 0x09U};

    init_ok(&dev, &iface);
    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_10HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_ONE_SHOT, XY_OK);
    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_10HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_ONE_SHOT, XY_OK);
    queue_read8(LPS22HB_STATUS, 0x00U, XY_OK);
    queue_read8(LPS22HB_STATUS, (uint8_t)(LPS22HB_P_DA | LPS22HB_T_DA), XY_OK);
    queue_read(LPS22HB_PRESS_OUT_XL, sample, sizeof(sample), XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_measure(&dev, &data, 20U));
    TEST_ASSERT_EQUAL_UINT32(45U, g_delay_total);

    setUp();
    dev.is_initialized = true;
    dev.interface = &iface;
    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_10HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_ONE_SHOT, XY_OK);
    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_10HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_ONE_SHOT, XY_OK);
    queue_read8(LPS22HB_STATUS, 0x00U, XY_OK);
    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_10HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_ONE_SHOT, XY_OK);
    TEST_ASSERT_EQUAL_INT(-2, xy_lps22hb_measure(&dev, &data, 5U));
}

static void test_controls_fifo_lpf_threshold_interrupt_and_deinit(void)
{
    xy_lps22hb_dev_t dev;
    xy_interface_dev_t iface = fake_interface();
    uint8_t low[2] = {0x34U, 0x12U};
    uint8_t high[2] = {0x78U, 0x56U};

    init_ok(&dev, &iface);
    TEST_ASSERT_TRUE(xy_lps22hb_is_ready(&dev));
    TEST_ASSERT_EQUAL_PTR(&dev.last_data, xy_lps22hb_get_last_data(&dev));

    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_10HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_25HZ, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_set_odr(&dev, XY_LPS22HB_ODR_25HZ));
    TEST_ASSERT_EQUAL_INT(XY_LPS22HB_ODR_25HZ, dev.config.odr);

    queue_read8(LPS22HB_CTRL_REG1, 0xFFU, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, (uint8_t)((0xFFU & ~(LPS22HB_EN_LPFP | LPS22HB_LPFP_CFG_MASK)) | LPS22HB_EN_LPFP | XY_LPS22HB_LPF_ODR_32), XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_configure_lpf(&dev, true, XY_LPS22HB_LPF_ODR_32));

    queue_write(LPS22HB_FIFO_CTRL, (uint8_t[]){(uint8_t)(XY_LPS22HB_FIFO_STREAM | 3U)}, 1U, XY_OK);
    queue_read8(LPS22HB_CTRL_REG2, 0x00U, XY_OK);
    queue_write8(LPS22HB_CTRL_REG2, LPS22HB_FIFO_EN, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_configure_fifo(&dev, XY_LPS22HB_FIFO_STREAM, 3U));
    TEST_ASSERT_TRUE(dev.config.enable_fifo);

    queue_write(LPS22HB_THS_P_L, low, sizeof(low), XY_OK);
    queue_write(LPS22HB_THS_P_H, high, sizeof(high), XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_configure_threshold(&dev, 0x1234U, 0x5678U));

    queue_read8(LPS22HB_INT_SOURCE, 0x01U, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_clear_interrupt(&dev));

    queue_read8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_25HZ, XY_OK);
    queue_write8(LPS22HB_CTRL_REG1, XY_LPS22HB_ODR_ONE_SHOT, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_lps22hb_deinit(&dev));
    TEST_ASSERT_FALSE(dev.is_initialized);

    TEST_ASSERT_FALSE(xy_lps22hb_is_ready(NULL));
    TEST_ASSERT_NULL(xy_lps22hb_get_last_data(NULL));
}

static void test_pressure_altitude_helpers_and_invalid_inputs(void)
{
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, xy_lps22hb_pressure_to_altitude(1013.25f, 1013.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1013.25f, xy_lps22hb_altitude_to_pressure(0.0f, 1013.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, xy_lps22hb_pressure_to_altitude(0.0f, 1013.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, xy_lps22hb_altitude_to_pressure(10.0f, 0.0f));

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_start_single(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_start_continuous(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_stop(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_check_data_ready(NULL, &(bool){false}));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_check_data_ready(&(xy_lps22hb_dev_t){0}, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_lps22hb_auto_zero(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_default_config_resets_and_programs_registers);
    RUN_TEST(test_init_rejects_bad_whoami_and_propagates_reset_timeout);
    RUN_TEST(test_read_data_converts_pressure_temperature_and_offsets);
    RUN_TEST(test_measure_waits_for_data_ready_and_handles_timeout);
    RUN_TEST(test_controls_fifo_lpf_threshold_interrupt_and_deinit);
    RUN_TEST(test_pressure_altitude_helpers_and_invalid_inputs);
    return UNITY_END();
}
