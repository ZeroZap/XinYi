#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_bq25620.h"
#include "xy_device.h"
#include "xy_os_tick.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t reg;
    uint8_t value;
    xy_error_t ret;
} read_op_t;

typedef struct {
    uint8_t reg;
    uint8_t value;
    xy_error_t ret;
} write_op_t;

static read_op_t g_reads[96];
static write_op_t g_writes[64];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static uint16_t g_last_addr;
static uint32_t g_last_timeout;

static void queue_read8(uint8_t reg, uint8_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    g_reads[g_read_count].reg = reg;
    g_reads[g_read_count].value = value;
    g_reads[g_read_count].ret = ret;
    g_read_count++;
}

static void queue_write8(uint8_t reg, uint8_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    g_writes[g_write_count].reg = reg;
    g_writes[g_write_count].value = value;
    g_writes[g_write_count].ret = ret;
    g_write_count++;
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
    g_last_addr = addr;
    g_last_timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_index);
    TEST_ASSERT_EQUAL_UINT8(g_reads[g_read_index].reg, reg);

    xy_error_t ret = g_reads[g_read_index].ret;
    if (ret == XY_DEVICE_OK) {
        *data = g_reads[g_read_index].value;
    }
    g_read_index++;
    return ret;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(2U, len);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_index);
    TEST_ASSERT_EQUAL_UINT8(g_writes[g_write_index].reg, data[0]);
    TEST_ASSERT_EQUAL_UINT8(g_writes[g_write_index].value, data[1]);

    return g_writes[g_write_index++].ret;
}

uint32_t xy_os_tick_get(void)
{
    return 0x12345678U;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_last_addr = 0;
    g_last_timeout = 0;
}

void tearDown(void)
{
}

static xy_bq25620_config_t test_config(void)
{
    xy_bq25620_config_t cfg = {
        .vbat_reg_mv = 4200,
        .ichg_ma = 1000,
        .iprecharge_ma = 100,
        .iterm_ma = 150,
        .vindpm_mv = 4500,
        .ivlim_ma = 1500,
        .recharge_mv = 100,
        .enable_auto_recharge = true,
    };
    return cfg;
}

static void queue_init_ok(const xy_bq25620_config_t *cfg)
{
    uint8_t ichg_bits = (uint8_t)(((cfg->ichg_ma - 100U) / 100U) & 0x1FU);
    uint8_t ipre_bits = (uint8_t)(((cfg->iprecharge_ma - 50U) / 50U) & 0x0FU);
    uint8_t iterm_bits = (uint8_t)(((cfg->iterm_ma - 50U) / 50U) & 0x0FU);
    uint8_t vreg_bits = (uint8_t)(((cfg->vbat_reg_mv - 3600U) / 10U) & 0x3FU);
    uint8_t vindpm_bits = (uint8_t)(((cfg->vindpm_mv - 3900U) / 100U) & 0x0FU);
    uint8_t ivlim_bits = (uint8_t)(((cfg->ivlim_ma - 500U) / 100U) & 0x0FU);
    uint8_t recharge_bits = (uint8_t)((cfg->recharge_mv / 100U) & 0x03U);

    queue_read8(BQ25620_REG_PART_ID, BQ25620_PART_ID_VALUE, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_DEV_ID, BQ25620_DEV_ID_VALUE, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_CHG_CTRL0, 0x03U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL0, (uint8_t)(0x03U | (ichg_bits << 3)), XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL1, (uint8_t)((ipre_bits << 4) | iterm_bits), XY_DEVICE_OK);
    queue_read8(BQ25620_REG_CHG_CTRL2, 0x01U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL2, (uint8_t)(0x01U | (vreg_bits << 2)), XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL3, (uint8_t)((vindpm_bits << 4) | ivlim_bits), XY_DEVICE_OK);
    queue_read8(BQ25620_REG_CHG_CTRL4, 0x01U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL4, (uint8_t)(0x01U | (cfg->enable_auto_recharge ? 0x80U : 0x00U)), XY_DEVICE_OK);
    queue_read8(BQ25620_REG_CHG_CTRL5, 0xF0U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL5, (uint8_t)(0xF0U | recharge_bits), XY_DEVICE_OK);
    queue_read8(BQ25620_REG_CHG_STAT, 0x04U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_STAT, 0x84U, XY_DEVICE_OK);
}

static void init_ok(xy_bq25620_t *bq, int *bus, const xy_bq25620_config_t *cfg)
{
    queue_init_ok(cfg);
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_init(bq, bus, cfg));
    TEST_ASSERT_TRUE(bq->initialized);
    TEST_ASSERT_EQUAL_UINT16(BQ25620_ADDR, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(400U, g_last_timeout);
}

static void test_init_rejects_invalid_inputs_and_configures_registers(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_init(NULL, &bus, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_init(&bq, NULL, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_init(&bq, &bus, NULL));

    init_ok(&bq, &bus, &cfg);
    TEST_ASSERT_EQUAL_UINT(7U, g_read_index);
    TEST_ASSERT_EQUAL_UINT(7U, g_write_index);
    TEST_ASSERT_EQUAL_MEMORY(&cfg, &bq.config, sizeof(cfg));
}

static void test_init_reports_id_and_config_failures(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    int bus;

    queue_read8(BQ25620_REG_PART_ID, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_NOT_FOUND, xy_bq25620_init(&bq, &bus, &cfg));

    queue_read8(BQ25620_REG_PART_ID, BQ25620_PART_ID_VALUE, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_DEV_ID, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_NOT_FOUND, xy_bq25620_init(&bq, &bus, &cfg));

    queue_read8(BQ25620_REG_PART_ID, BQ25620_PART_ID_VALUE, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_DEV_ID, BQ25620_DEV_ID_VALUE, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_CHG_CTRL0, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_init(&bq, &bus, &cfg));
}

static void test_read_parses_status_fault_and_timestamp(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_read(NULL));
    memset(&bq, 0, sizeof(bq));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_read(&bq));

    init_ok(&bq, &bus, &cfg);
    queue_read8(BQ25620_REG_CHG_STAT, 0xACU, XY_DEVICE_OK); /* enabled + fast charge + PG + DPM */
    queue_read8(BQ25620_REG_FAULT, 0x40U, XY_DEVICE_OK);    /* thermal */
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_read(&bq));
    TEST_ASSERT_EQUAL_INT(XY_BQ_CHG_STATE_FAST_CHARGE, bq.data.chg_state);
    TEST_ASSERT_TRUE(bq.data.vbus_present);
    TEST_ASSERT_TRUE(bq.data.dpm_active);
    TEST_ASSERT_TRUE(bq.data.chg_enabled);
    TEST_ASSERT_EQUAL_INT(XY_BQ_FAULT_THERMAL, bq.data.fault);
    TEST_ASSERT_EQUAL_UINT32(0x12345678U, bq.data.timestamp);

    queue_read8(BQ25620_REG_CHG_STAT, 0x30U, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_FAULT, 0x80U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_read(&bq));
    TEST_ASSERT_EQUAL_INT(XY_BQ_CHG_STATE_CHARGE_DONE, bq.data.chg_state);
    TEST_ASSERT_FALSE(bq.data.vbus_present);
    TEST_ASSERT_FALSE(bq.data.dpm_active);
    TEST_ASSERT_FALSE(bq.data.chg_enabled);
    TEST_ASSERT_EQUAL_INT(XY_BQ_FAULT_BAT_OVP, bq.data.fault);

    queue_read8(BQ25620_REG_CHG_STAT, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_read(&bq));
}

static void test_read_fault_failure_keeps_previous_fault_and_updates_status(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    int bus;

    init_ok(&bq, &bus, &cfg);
    bq.data.fault = XY_BQ_FAULT_THERMAL;
    queue_read8(BQ25620_REG_CHG_STAT, 0x98U, XY_DEVICE_OK);   /* enabled + precharge + PG */
    queue_read8(BQ25620_REG_FAULT, 0x00U, XY_DEVICE_ERROR);

    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_read(&bq));
    TEST_ASSERT_EQUAL_INT(XY_BQ_CHG_STATE_PRECHARGE, bq.data.chg_state);
    TEST_ASSERT_TRUE(bq.data.vbus_present);
    TEST_ASSERT_FALSE(bq.data.dpm_active);
    TEST_ASSERT_TRUE(bq.data.chg_enabled);
    TEST_ASSERT_EQUAL_INT(XY_BQ_FAULT_THERMAL, bq.data.fault);
    TEST_ASSERT_EQUAL_UINT32(0x12345678U, bq.data.timestamp);
}

static void test_helpers_propagate_read_failures_without_overwriting_outputs(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    xy_bq_chg_state_t state = XY_BQ_CHG_STATE_CHARGE_DONE;
    xy_bq_fault_t fault = XY_BQ_FAULT_BAT_OVP;
    int bus;

    init_ok(&bq, &bus, &cfg);

    queue_read8(BQ25620_REG_CHG_STAT, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_get_chg_state(&bq, &state));
    TEST_ASSERT_EQUAL_INT(XY_BQ_CHG_STATE_CHARGE_DONE, state);

    queue_read8(BQ25620_REG_CHG_STAT, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_get_fault(&bq, &fault));
    TEST_ASSERT_EQUAL_INT(XY_BQ_FAULT_BAT_OVP, fault);

    bq.data.vbus_present = true;
    queue_read8(BQ25620_REG_CHG_STAT, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_TRUE(xy_bq25620_is_vbus_present(&bq));
}

static void test_control_helpers_propagate_update_bit_failures(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    int bus;

    init_ok(&bq, &bus, &cfg);

    queue_read8(BQ25620_REG_CHG_STAT, 0x84U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_stop_charging(&bq));

    queue_read8(BQ25620_REG_CHG_STAT, 0x04U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_STAT, 0x84U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_start_charging(&bq));

    queue_read8(BQ25620_REG_CHG_CTRL0, 0x07U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_set_charge_current(&bq, 1000));

    queue_read8(BQ25620_REG_CHG_CTRL2, 0x01U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL2, 0xF1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_set_battery_voltage(&bq, 4200));

    queue_read8(BQ25620_REG_FAULT, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bq25620_reset_fault(&bq));
}

static void test_getters_and_control_helpers(void)
{
    xy_bq25620_t bq;
    xy_bq25620_config_t cfg = test_config();
    xy_bq_chg_state_t state = XY_BQ_CHG_STATE_IDLE;
    xy_bq_fault_t fault = XY_BQ_FAULT_NONE;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_get_chg_state(NULL, &state));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_get_chg_state(&bq, NULL));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_get_fault(NULL, &fault));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_get_fault(&bq, NULL));
    TEST_ASSERT_FALSE(xy_bq25620_is_vbus_present(NULL));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_enable_charge(NULL, true));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_set_charge_current(NULL, 1000));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_set_charge_current(&bq, 99));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_set_charge_current(&bq, 3201));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_set_battery_voltage(NULL, 4200));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_set_battery_voltage(&bq, 3599));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_set_battery_voltage(&bq, 4201));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_reset_fault(NULL));
    TEST_ASSERT_EQUAL_INT(XY_BQ_INVALID_PARAM, xy_bq25620_deinit(NULL));

    init_ok(&bq, &bus, &cfg);

    queue_read8(BQ25620_REG_CHG_STAT, 0x18U, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_FAULT, 0x20U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_get_chg_state(&bq, &state));
    TEST_ASSERT_EQUAL_INT(XY_BQ_CHG_STATE_PRECHARGE, state);

    queue_read8(BQ25620_REG_CHG_STAT, 0x00U, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_FAULT, 0x60U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_get_fault(&bq, &fault));
    TEST_ASSERT_EQUAL_INT(XY_BQ_FAULT_CHG_TIMEOUT, fault);

    queue_read8(BQ25620_REG_CHG_STAT, 0x08U, XY_DEVICE_OK);
    queue_read8(BQ25620_REG_FAULT, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_TRUE(xy_bq25620_is_vbus_present(&bq));

    queue_read8(BQ25620_REG_CHG_STAT, 0x84U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_STAT, 0x04U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_stop_charging(&bq));

    queue_read8(BQ25620_REG_CHG_STAT, 0x04U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_STAT, 0x84U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_start_charging(&bq));

    queue_read8(BQ25620_REG_CHG_CTRL0, 0x07U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL0, 0x4FU, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_current(&bq, 1000));

    queue_read8(BQ25620_REG_CHG_CTRL2, 0x01U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_CTRL2, 0xF1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_battery_voltage(&bq, 4200));

    queue_read8(BQ25620_REG_FAULT, 0x20U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_reset_fault(&bq));

    queue_read8(BQ25620_REG_CHG_STAT, 0x84U, XY_DEVICE_OK);
    queue_write8(BQ25620_REG_CHG_STAT, 0x04U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BQ_OK, xy_bq25620_deinit(&bq));
    TEST_ASSERT_FALSE(bq.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_configures_registers);
    RUN_TEST(test_init_reports_id_and_config_failures);
    RUN_TEST(test_read_parses_status_fault_and_timestamp);
    RUN_TEST(test_read_fault_failure_keeps_previous_fault_and_updates_status);
    RUN_TEST(test_helpers_propagate_read_failures_without_overwriting_outputs);
    RUN_TEST(test_control_helpers_propagate_update_bit_failures);
    RUN_TEST(test_getters_and_control_helpers);
    return UNITY_END();
}
