#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_mlx90614.h"
#include "xy_device.h"
#include "xy_hal_error.h"

static uint8_t g_read_regs[256][3];
static uint8_t g_read_fail_reg[256];
static uint8_t g_last_addr;

static uint8_t mlx_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x07U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void set_reg_word(uint8_t reg, uint16_t value)
{
    g_read_regs[reg][0] = (uint8_t)(value & 0xFFU);
    g_read_regs[reg][1] = (uint8_t)(value >> 8);
    g_read_regs[reg][2] = mlx_crc8(g_read_regs[reg], 2U);
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(i2c_handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    g_last_addr = (uint8_t)addr;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(3U, len);

    if (g_read_fail_reg[reg]) {
        return XY_DEVICE_ERROR;
    }

    memcpy(data, g_read_regs[reg], len);
    return XY_DEVICE_OK;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

void setUp(void)
{
    memset(g_read_regs, 0, sizeof(g_read_regs));
    memset(g_read_fail_reg, 0, sizeof(g_read_fail_reg));
    g_last_addr = 0;

    set_reg_word(0x0C, 0xBEEF);
    set_reg_word(MLX90614_RAM_TA, 15000U);    /* 300.00 K -> 26.85 C */
    set_reg_word(MLX90614_RAM_TOBJ1, 15250U); /* 305.00 K -> 31.85 C */
    set_reg_word(MLX90614_RAM_TOBJ2, 15500U); /* 310.00 K -> 36.85 C */
    set_reg_word(MLX90614_EMISSIVITY, 62257U); /* ~0.950 */
}

void tearDown(void)
{
}

static void test_init_rejects_invalid_inputs_and_uses_default_address(void)
{
    xy_mlx90614_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_init(NULL, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_init(&dev, NULL, MLX90614_ADDR_DEFAULT));

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(MLX90614_ADDR_DEFAULT, g_last_addr);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
}

static void test_read_all_converts_temperature_registers_and_single_channel_fallback(void)
{
    xy_mlx90614_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    g_read_fail_reg[MLX90614_RAM_TOBJ2] = 1U;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_read_all(&dev));
    TEST_ASSERT_EQUAL_INT16(2685, dev.ta);
    TEST_ASSERT_EQUAL_INT16(3185, dev.tobj1);
    TEST_ASSERT_EQUAL_INT16(3185, dev.tobj2);
}

static void test_bad_pec_rejects_temperature_read_without_updating_output(void)
{
    xy_mlx90614_t dev;
    int16_t ta = 1234;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    g_read_regs[MLX90614_RAM_TA][2] ^= 0x55U;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_ERROR, xy_mlx90614_read_ambient(&dev, &ta));
    TEST_ASSERT_EQUAL_INT16(1234, ta);
}

static void test_read_ambient_success_and_invalid_output_paths(void)
{
    xy_mlx90614_t dev;
    int16_t ta = -1234;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_read_ambient(NULL, &ta));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_read_ambient(&dev, NULL));

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_read_ambient(&dev, &ta));
    TEST_ASSERT_EQUAL_INT16(2685, ta);
}

static void test_read_all_failures_preserve_cached_temperatures(void)
{
    xy_mlx90614_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    dev.ta = 111;
    dev.tobj1 = 222;
    dev.tobj2 = 333;

    g_read_fail_reg[MLX90614_RAM_TA] = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mlx90614_read_all(&dev));
    TEST_ASSERT_EQUAL_INT16(111, dev.ta);
    TEST_ASSERT_EQUAL_INT16(222, dev.tobj1);
    TEST_ASSERT_EQUAL_INT16(333, dev.tobj2);

    g_read_fail_reg[MLX90614_RAM_TA] = 0U;
    g_read_fail_reg[MLX90614_RAM_TOBJ1] = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mlx90614_read_all(&dev));
    TEST_ASSERT_EQUAL_INT16(111, dev.ta);
    TEST_ASSERT_EQUAL_INT16(222, dev.tobj1);
    TEST_ASSERT_EQUAL_INT16(333, dev.tobj2);
}

static void test_init_reports_not_found_when_id_read_fails(void)
{
    xy_mlx90614_t dev;
    int fake_bus;

    g_read_fail_reg[0x0C] = 1U;
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_NOT_FOUND,
                          xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
}

static void test_read_all_rejects_invalid_or_uninitialized_device(void)
{
    xy_mlx90614_t dev;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_read_all(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_read_all(&dev));
}

static void test_read_object1_updates_output_only_on_success(void)
{
    xy_mlx90614_t dev;
    int16_t tobj = -1234;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_read_object1(NULL, &tobj));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_read_object1(&dev, NULL));

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_read_object1(&dev, &tobj));
    TEST_ASSERT_EQUAL_INT16(3185, tobj);

    g_read_regs[MLX90614_RAM_TA][2] ^= 0x01U;
    tobj = -1234;
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_ERROR, xy_mlx90614_read_object1(&dev, &tobj));
    TEST_ASSERT_EQUAL_INT16(-1234, tobj);
}

static void test_deinit_rejects_null_and_clears_initialized_flag(void)
{
    xy_mlx90614_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
}

static void test_emissivity_get_converts_calibration_and_falls_back_on_i2c_error(void)
{
    xy_mlx90614_t dev;
    uint16_t emissivity = 0;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_get_emissivity(NULL, &emissivity));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_get_emissivity(&dev, NULL));

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_get_emissivity(&dev, &emissivity));
    TEST_ASSERT_EQUAL_UINT16(949U, emissivity);

    emissivity = 0;
    g_read_fail_reg[MLX90614_EMISSIVITY] = 1U;
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_get_emissivity(&dev, &emissivity));
    TEST_ASSERT_EQUAL_UINT16(950U, emissivity);
}

static void test_emissivity_get_falls_back_on_bad_pec(void)
{
    xy_mlx90614_t dev;
    uint16_t emissivity = 123U;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    g_read_regs[MLX90614_EMISSIVITY][2] ^= 0x80U;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_get_emissivity(&dev, &emissivity));
    TEST_ASSERT_EQUAL_UINT16(950U, emissivity);
}

static void test_set_emissivity_validates_range_and_reports_unsupported_write(void)
{
    xy_mlx90614_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_set_emissivity(NULL, 950U));

    TEST_ASSERT_EQUAL_INT(XY_MLX90614_OK, xy_mlx90614_init(&dev, &fake_bus, MLX90614_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_set_emissivity(&dev, 99U));
    TEST_ASSERT_EQUAL_INT(XY_MLX90614_INVALID_PARAM, xy_mlx90614_set_emissivity(&dev, 1001U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_NOT_SUPPORTED, xy_mlx90614_set_emissivity(&dev, 950U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_uses_default_address);
    RUN_TEST(test_init_reports_not_found_when_id_read_fails);
    RUN_TEST(test_read_all_rejects_invalid_or_uninitialized_device);
    RUN_TEST(test_read_all_converts_temperature_registers_and_single_channel_fallback);
    RUN_TEST(test_bad_pec_rejects_temperature_read_without_updating_output);
    RUN_TEST(test_read_ambient_success_and_invalid_output_paths);
    RUN_TEST(test_read_all_failures_preserve_cached_temperatures);
    RUN_TEST(test_read_object1_updates_output_only_on_success);
    RUN_TEST(test_deinit_rejects_null_and_clears_initialized_flag);
    RUN_TEST(test_emissivity_get_converts_calibration_and_falls_back_on_i2c_error);
    RUN_TEST(test_emissivity_get_falls_back_on_bad_pec);
    RUN_TEST(test_set_emissivity_validates_range_and_reports_unsupported_write);
    return UNITY_END();
}
