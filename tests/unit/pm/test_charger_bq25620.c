#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"
#include "unity.h"
#include "xy_bq25620.h"
#include "xy_hal_i2c.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_i2c_master_transmit, void *, uint16_t,
                const uint8_t *, size_t, uint32_t)
FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_i2c_master_receive, void *, uint16_t,
                uint8_t *, size_t, uint32_t)

static uint8_t g_regs[0x20];
static uint8_t g_selected_reg;
static unsigned g_tx_count;
static unsigned g_rx_count;
static void *g_expected_i2c = (void *)0x1234;

static xy_hal_error_t fake_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                               const uint8_t *data, size_t len,
                                               uint32_t timeout);
static xy_hal_error_t fake_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                              uint8_t *data, size_t len,
                                              uint32_t timeout);

void setUp(void)
{
    RESET_FAKE(xy_hal_i2c_master_transmit);
    RESET_FAKE(xy_hal_i2c_master_receive);
    FFF_RESET_HISTORY();

    xy_hal_i2c_master_transmit_fake.custom_fake = fake_i2c_master_transmit;
    xy_hal_i2c_master_receive_fake.custom_fake = fake_i2c_master_receive;
}

void tearDown(void)
{
}

static void reset_fake_i2c(void)
{
    memset(g_regs, 0, sizeof(g_regs));
    g_regs[BQ25620_REG_DEVICE_ID] = BQ25620_PART_NUMBER;
    g_selected_reg = 0;
    g_tx_count = 0;
    g_rx_count = 0;
}

static xy_hal_error_t fake_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                               const uint8_t *data, size_t len,
                                               uint32_t timeout)
{
    (void)dev_addr;
    (void)timeout;

    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, i2c);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_TRUE(len == 1 || len == 2);
    g_tx_count++;

    g_selected_reg = data[0];
    if (len == 2) {
        TEST_ASSERT_LESS_THAN(sizeof(g_regs), g_selected_reg);
        g_regs[g_selected_reg] = data[1];
    }

    return XY_HAL_OK;
}

static xy_hal_error_t fake_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                              uint8_t *data, size_t len,
                                              uint32_t timeout)
{
    (void)dev_addr;
    (void)timeout;

    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, i2c);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_regs), g_selected_reg + len);
    memcpy(data, &g_regs[g_selected_reg], len);
    g_rx_count++;
    return XY_HAL_OK;
}

static void test_null_param_validation(void)
{
    xy_bq25620_t dev;
    uint8_t id;
    xy_charger_status_t status;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_init(NULL, g_expected_i2c, 0x6A));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_init(&dev, NULL, 0x6A));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_read_reg(NULL, BQ25620_REG_DEVICE_ID, &id));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_write_reg(NULL, BQ25620_REG_DEVICE_ID, 0));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_get_device_id(NULL, &id));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_get_device_id(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_get_status(NULL, &status));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_get_status(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_set_charge_current(NULL, 1000));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_set_charge_voltage(NULL, 4200));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_set_input_limit(NULL, 500));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_start_charge(NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_stop_charge(NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_deinit(NULL));
}

static void test_init_and_register_io(void)
{
    xy_bq25620_t dev;
    uint8_t value = 0;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6A));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, dev.i2c_handle);
    TEST_ASSERT_EQUAL_HEX16(0x6A, dev.i2c_addr);
    TEST_ASSERT_EQUAL_PTR(&dev, dev.base.hw_data);
    TEST_ASSERT_NOT_NULL(dev.base.hw_read_reg);
    TEST_ASSERT_EQUAL_UINT(1U, g_tx_count);
    TEST_ASSERT_EQUAL_UINT(1U, g_rx_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_master_receive_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, xy_hal_i2c_master_transmit_fake.arg0_val);
    TEST_ASSERT_EQUAL_HEX16(0x6A, xy_hal_i2c_master_transmit_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_master_transmit_fake.arg3_val);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_device_id(&dev, &value));
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_i2c_master_receive_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(BQ25620_PART_NUMBER, value);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_write_reg(&dev, BQ25620_REG_CHG_CTRL_6, 0x55));
    TEST_ASSERT_EQUAL_UINT(3U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_i2c_master_transmit_fake.arg3_val);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_read_reg(&dev, BQ25620_REG_CHG_CTRL_6, &value));
    TEST_ASSERT_EQUAL_UINT(4U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(3U, xy_hal_i2c_master_receive_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0x55, value);
}

static void test_status_decoding(void)
{
    xy_bq25620_t dev;
    xy_charger_status_t status;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6A));

    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_FAST | BQ25620_STAT_PG;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_THERMAL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_STATE_FAST_CHARGE, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_FAULT_THERMAL, status.fault);
    TEST_ASSERT_TRUE(status.power_good);
    TEST_ASSERT_TRUE(status.charging);
    TEST_ASSERT_FALSE(status.done);

    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_DONE;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_NORMAL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_STATE_CHARGE_DONE, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_FAULT_NONE, status.fault);
    TEST_ASSERT_FALSE(status.charging);
    TEST_ASSERT_TRUE(status.done);
}

static void test_config_and_clamping(void)
{
    xy_bq25620_t dev;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6A));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_current(&dev, 32));
    TEST_ASSERT_EQUAL_HEX8(0U, g_regs[BQ25620_REG_CHG_CTRL_1]);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_current(&dev, 128));
    TEST_ASSERT_EQUAL_HEX8(1U, g_regs[BQ25620_REG_CHG_CTRL_1]);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_current(&dev, 6000));
    TEST_ASSERT_EQUAL_HEX8((BQ25620_ICHG_MAX_mA - BQ25620_ICHG_MIN_mA) / BQ25620_ICHG_STEP_mA,
                           g_regs[BQ25620_REG_CHG_CTRL_1]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_voltage(&dev, 3400));
    TEST_ASSERT_EQUAL_HEX8(0U, g_regs[BQ25620_REG_CHG_CTRL_3]);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_voltage(&dev, 4200));
    TEST_ASSERT_EQUAL_HEX8(70U, g_regs[BQ25620_REG_CHG_CTRL_3]);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_voltage(&dev, 5000));
    TEST_ASSERT_EQUAL_HEX8((BQ25620_VREG_MAX_mV - BQ25620_VREG_MIN_mV) / BQ25620_VREG_STEP_mV,
                           g_regs[BQ25620_REG_CHG_CTRL_3]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_input_limit(&dev, 50));
    TEST_ASSERT_EQUAL_HEX8(BQ25620_EN_ILIM, g_regs[BQ25620_REG_CHG_CTRL_4]);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_input_limit(&dev, 500));
    TEST_ASSERT_EQUAL_HEX8(BQ25620_EN_ILIM | 4U, g_regs[BQ25620_REG_CHG_CTRL_4]);
}

static void test_start_stop_and_deinit(void)
{
    xy_bq25620_t dev;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6A));

    g_regs[BQ25620_REG_CHG_CTRL_0] = 0x01;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_start_charge(&dev));
    TEST_ASSERT_BITS_HIGH(BQ25620_EN_CHG, g_regs[BQ25620_REG_CHG_CTRL_0]);
    TEST_ASSERT_BITS_HIGH(0x01, g_regs[BQ25620_REG_CHG_CTRL_0]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_stop_charge(&dev));
    TEST_ASSERT_BITS_LOW(BQ25620_EN_CHG, g_regs[BQ25620_REG_CHG_CTRL_0]);
    TEST_ASSERT_BITS_HIGH(0x01, g_regs[BQ25620_REG_CHG_CTRL_0]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_param_validation);
    RUN_TEST(test_init_and_register_io);
    RUN_TEST(test_status_decoding);
    RUN_TEST(test_config_and_clamping);
    RUN_TEST(test_start_stop_and_deinit);
    return UNITY_END();
}
