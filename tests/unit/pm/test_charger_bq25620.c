#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_bq25620.h"
#include "xy_hal_i2c.h"

static uint8_t g_regs[0x20];
static uint8_t g_selected_reg;
static unsigned g_tx_count;
static unsigned g_rx_count;
static void *g_expected_i2c = (void *)0x1234;

static void reset_fake_i2c(void)
{
    memset(g_regs, 0, sizeof(g_regs));
    g_regs[BQ25620_REG_DEVICE_ID] = BQ25620_PART_NUMBER;
    g_selected_reg = 0;
    g_tx_count = 0;
    g_rx_count = 0;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    (void)dev_addr;
    (void)timeout;

    assert(i2c == g_expected_i2c);
    assert(data != NULL);
    assert(len == 1 || len == 2);
    g_tx_count++;

    g_selected_reg = data[0];
    if (len == 2) {
        assert(g_selected_reg < sizeof(g_regs));
        g_regs[g_selected_reg] = data[1];
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                         uint8_t *data, size_t len,
                                         uint32_t timeout)
{
    (void)dev_addr;
    (void)timeout;

    assert(i2c == g_expected_i2c);
    assert(data != NULL);
    assert(g_selected_reg + len <= sizeof(g_regs));
    memcpy(data, &g_regs[g_selected_reg], len);
    g_rx_count++;
    return XY_HAL_OK;
}

static void test_null_param_validation(void)
{
    xy_bq25620_t dev;
    uint8_t id;
    xy_charger_status_t status;

    assert(xy_bq25620_init(NULL, g_expected_i2c, 0x6A) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_init(&dev, NULL, 0x6A) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_read_reg(NULL, BQ25620_REG_DEVICE_ID, &id) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, NULL) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_write_reg(NULL, BQ25620_REG_DEVICE_ID, 0) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_get_device_id(NULL, &id) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_get_device_id(&dev, NULL) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_get_status(NULL, &status) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_get_status(&dev, NULL) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_set_charge_current(NULL, 1000) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_set_charge_voltage(NULL, 4200) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_set_input_limit(NULL, 500) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_start_charge(NULL) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_stop_charge(NULL) == XY_DEVICE_INVALID_PARAM);
    assert(xy_bq25620_deinit(NULL) == XY_DEVICE_INVALID_PARAM);
}

static void test_init_and_register_io(void)
{
    xy_bq25620_t dev;
    uint8_t value = 0;

    reset_fake_i2c();
    assert(xy_bq25620_init(&dev, g_expected_i2c, 0x6A) == XY_DEVICE_OK);
    assert(dev.initialized);
    assert(dev.i2c_handle == g_expected_i2c);
    assert(dev.i2c_addr == 0x6A);
    assert(dev.base.hw_data == &dev);
    assert(dev.base.hw_read_reg != NULL);
    assert(g_tx_count == 1U);
    assert(g_rx_count == 1U);

    assert(xy_bq25620_get_device_id(&dev, &value) == XY_DEVICE_OK);
    assert(value == BQ25620_PART_NUMBER);

    assert(xy_bq25620_write_reg(&dev, BQ25620_REG_CHG_CTRL_6, 0x55) == XY_DEVICE_OK);
    assert(xy_bq25620_read_reg(&dev, BQ25620_REG_CHG_CTRL_6, &value) == XY_DEVICE_OK);
    assert(value == 0x55);
}

static void test_status_decoding(void)
{
    xy_bq25620_t dev;
    xy_charger_status_t status;

    reset_fake_i2c();
    assert(xy_bq25620_init(&dev, g_expected_i2c, 0x6A) == XY_DEVICE_OK);

    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_FAST | BQ25620_STAT_PG;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_THERMAL;
    assert(xy_bq25620_get_status(&dev, &status) == XY_DEVICE_OK);
    assert(status.state == XY_CHARGER_STATE_FAST_CHARGE);
    assert(status.fault == XY_CHARGER_FAULT_THERMAL);
    assert(status.power_good);
    assert(status.charging);
    assert(!status.done);

    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_DONE;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_NORMAL;
    assert(xy_bq25620_get_status(&dev, &status) == XY_DEVICE_OK);
    assert(status.state == XY_CHARGER_STATE_CHARGE_DONE);
    assert(status.fault == XY_CHARGER_FAULT_NONE);
    assert(!status.charging);
    assert(status.done);
}

static void test_config_and_clamping(void)
{
    xy_bq25620_t dev;

    reset_fake_i2c();
    assert(xy_bq25620_init(&dev, g_expected_i2c, 0x6A) == XY_DEVICE_OK);

    assert(xy_bq25620_set_charge_current(&dev, 32) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_1] == 0U);
    assert(xy_bq25620_set_charge_current(&dev, 128) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_1] == 1U);
    assert(xy_bq25620_set_charge_current(&dev, 6000) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_1] == ((BQ25620_ICHG_MAX_mA - BQ25620_ICHG_MIN_mA) / BQ25620_ICHG_STEP_mA));

    assert(xy_bq25620_set_charge_voltage(&dev, 3400) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_3] == 0U);
    assert(xy_bq25620_set_charge_voltage(&dev, 4200) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_3] == 70U);
    assert(xy_bq25620_set_charge_voltage(&dev, 5000) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_3] == ((BQ25620_VREG_MAX_mV - BQ25620_VREG_MIN_mV) / BQ25620_VREG_STEP_mV));

    assert(xy_bq25620_set_input_limit(&dev, 50) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_4] == BQ25620_EN_ILIM);
    assert(xy_bq25620_set_input_limit(&dev, 500) == XY_DEVICE_OK);
    assert(g_regs[BQ25620_REG_CHG_CTRL_4] == (BQ25620_EN_ILIM | 4U));
}

static void test_start_stop_and_deinit(void)
{
    xy_bq25620_t dev;

    reset_fake_i2c();
    assert(xy_bq25620_init(&dev, g_expected_i2c, 0x6A) == XY_DEVICE_OK);

    g_regs[BQ25620_REG_CHG_CTRL_0] = 0x01;
    assert(xy_bq25620_start_charge(&dev) == XY_DEVICE_OK);
    assert((g_regs[BQ25620_REG_CHG_CTRL_0] & BQ25620_EN_CHG) != 0);
    assert((g_regs[BQ25620_REG_CHG_CTRL_0] & 0x01) != 0);

    assert(xy_bq25620_stop_charge(&dev) == XY_DEVICE_OK);
    assert((g_regs[BQ25620_REG_CHG_CTRL_0] & BQ25620_EN_CHG) == 0);
    assert((g_regs[BQ25620_REG_CHG_CTRL_0] & 0x01) != 0);

    assert(xy_bq25620_deinit(&dev) == XY_DEVICE_OK);
    assert(!dev.initialized);
}

int main(void)
{
    test_null_param_validation();
    test_init_and_register_io();
    test_status_decoding();
    test_config_and_clamping();
    test_start_stop_and_deinit();
    return 0;
}
