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

static uint8_t g_regs[0x40];
static uint8_t g_selected_reg;
static void *g_expected_i2c = (void *)0x1234;
static const uint16_t g_expected_i2c_addr = 0x6BU;
static unsigned g_fail_tx_call;
static unsigned g_fail_rx_call;
static xy_hal_error_t g_injected_error;

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
    g_regs[BQ25620_REG_DEVICE_ID] = BQ25620_PART_NUMBER | 0x02U;
    g_selected_reg = 0;
    g_fail_tx_call = 0U;
    g_fail_rx_call = 0U;
    g_injected_error = XY_HAL_ERROR;
}

static xy_hal_error_t fake_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                               const uint8_t *data, size_t len,
                                               uint32_t timeout)
{
    (void)timeout;

    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, i2c);
    TEST_ASSERT_EQUAL_HEX16(g_expected_i2c_addr, dev_addr);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(1U, len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(3U, len);

    if (g_fail_tx_call != 0U &&
        xy_hal_i2c_master_transmit_fake.call_count == g_fail_tx_call) {
        return g_injected_error;
    }

    g_selected_reg = data[0];
    if (len > 1U) {
        TEST_ASSERT_LESS_THAN(sizeof(g_regs), g_selected_reg);
        TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_regs), g_selected_reg + len - 1U);
        memcpy(&g_regs[g_selected_reg], &data[1], len - 1U);
    }

    return XY_HAL_OK;
}

static xy_hal_error_t fake_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                              uint8_t *data, size_t len,
                                              uint32_t timeout)
{
    (void)timeout;

    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, i2c);
    TEST_ASSERT_EQUAL_HEX16(g_expected_i2c_addr, dev_addr);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_regs), g_selected_reg + len);
    if (g_fail_rx_call != 0U &&
        xy_hal_i2c_master_receive_fake.call_count == g_fail_rx_call) {
        memset(data, 0x5A, len);
        return g_injected_error;
    }
    memcpy(data, &g_regs[g_selected_reg], len);
    return XY_HAL_OK;
}

static void set_reg_u16(uint8_t reg, uint16_t value)
{
    g_regs[reg] = (uint8_t)value;
    g_regs[reg + 1U] = (uint8_t)(value >> 8);
}

static uint16_t get_reg_u16(uint8_t reg)
{
    return (uint16_t)g_regs[reg] | ((uint16_t)g_regs[reg + 1U] << 8);
}

static void test_datasheet_register_map_contract(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x02U, BQ25620_REG_CHG_CTRL_1);
    TEST_ASSERT_EQUAL_HEX8(0x04U, BQ25620_REG_CHG_CTRL_3);
    TEST_ASSERT_EQUAL_HEX8(0x06U, BQ25620_REG_CHG_CTRL_4);
    TEST_ASSERT_EQUAL_HEX8(0x10U, BQ25620_REG_CHG_CTRL_2);
    TEST_ASSERT_EQUAL_HEX8(0x12U, BQ25620_REG_CHG_CTRL_5);
    TEST_ASSERT_EQUAL_HEX8(0x14U, BQ25620_REG_CHG_CTRL_0);
    TEST_ASSERT_EQUAL_HEX8(0x16U, BQ25620_REG_CHG_CTRL_6);
    TEST_ASSERT_EQUAL_HEX8(0x1DU, BQ25620_REG_ADC_STAT_0);
    TEST_ASSERT_EQUAL_HEX8(0x1EU, BQ25620_REG_CHG_STAT_0);
    TEST_ASSERT_EQUAL_HEX8(0x1FU, BQ25620_REG_CHG_STAT_1);
    TEST_ASSERT_EQUAL_UINT(80U, BQ25620_ICHG_STEP_mA);
    TEST_ASSERT_EQUAL_UINT(3520U, BQ25620_ICHG_MAX_mA);
    TEST_ASSERT_EQUAL_UINT(4800U, BQ25620_VREG_MAX_mV);
    TEST_ASSERT_EQUAL_UINT(20U, BQ25620_ILIM_STEP_mA);
    TEST_ASSERT_EQUAL_UINT(3200U, BQ25620_ILIM_MAX_mA);
}

static void test_null_param_validation(void)
{
    xy_bq25620_t dev;
    uint8_t id;
    xy_charger_device_status_t status;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_init(NULL, g_expected_i2c, 0x6B));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_init(&dev, NULL, 0x6B));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_read_reg(NULL, BQ25620_REG_DEVICE_ID, &id));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, NULL));
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
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6B));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.base.base.initialized);
    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, dev.i2c_handle);
    TEST_ASSERT_EQUAL_HEX16(0x6B, dev.i2c_addr);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_master_receive_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, xy_hal_i2c_master_transmit_fake.arg0_val);
    TEST_ASSERT_EQUAL_HEX16(0x6B, xy_hal_i2c_master_transmit_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_master_transmit_fake.arg3_val);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_device_id(&dev, &value));
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_i2c_master_receive_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(BQ25620_PART_NUMBER | 0x02U, value);

    g_regs[BQ25620_REG_CHG_CTRL_6] = 0x55U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_read_reg(&dev, BQ25620_REG_CHG_CTRL_6, &value));
    TEST_ASSERT_EQUAL_UINT(3U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(3U, xy_hal_i2c_master_receive_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0x55, value);
}

static void test_status_decoding(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6B));

    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_FAST | 0x03U;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_THERMAL;
    set_reg_u16(BQ25620_REG_CHG_CTRL_1, (160U / BQ25620_ICHG_STEP_mA) << 6U);
    set_reg_u16(BQ25620_REG_CHG_CTRL_3, (4200U / BQ25620_VREG_STEP_mV) << 3U);
    set_reg_u16(BQ25620_REG_CHG_CTRL_4, (500U / BQ25620_ILIM_STEP_mA) << 4U);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_STATE_FAULT, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_FAULT_THERMAL, status.fault);
    TEST_ASSERT_TRUE(status.power_good);
    TEST_ASSERT_FALSE(status.charging);
    TEST_ASSERT_FALSE(status.done);
    TEST_ASSERT_EQUAL_UINT32(160U, status.configured_charge_current);
    TEST_ASSERT_EQUAL_UINT32(4200U, status.configured_charge_voltage);
    TEST_ASSERT_EQUAL_UINT32(500U, status.configured_input_current_limit);

    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_TOPOFF;
    g_regs[BQ25620_REG_CHG_STAT_1] = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_STATE_CHARGE_DONE, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_FAULT_NONE, status.fault);
    TEST_ASSERT_FALSE(status.charging);
    TEST_ASSERT_TRUE(status.done);
}

static void test_unknown_status_codes_fail_closed(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_TOPOFF | 0x03U;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_SYS;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_STATE_FAULT, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_FAULT_UNKNOWN, status.fault);
    TEST_ASSERT_TRUE(status.power_good);
    TEST_ASSERT_FALSE(status.charging);
    TEST_ASSERT_FALSE(status.done);
}

static void test_known_fault_overrides_done_state(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_TOPOFF | 0x03U;
    g_regs[BQ25620_REG_CHG_STAT_1] = BQ25620_FAULT_BAT_OVP;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_STATE_FAULT, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_FAULT_BAT_OVP, status.fault);
    TEST_ASSERT_TRUE(status.power_good);
    TEST_ASSERT_FALSE(status.charging);
    TEST_ASSERT_FALSE(status.done);
}

static void test_unknown_ts_state_reports_unknown_fault(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_FAST | 0x03U;
    g_regs[BQ25620_REG_CHG_STAT_1] = 0x07U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_STATE_FAULT, status.state);
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_DEVICE_FAULT_UNKNOWN, status.fault);
    TEST_ASSERT_TRUE(status.power_good);
    TEST_ASSERT_FALSE(status.charging);
    TEST_ASSERT_FALSE(status.done);
}

static void test_status_accepts_datasheet_maximum_setpoint_encodings(void)
{
    static const struct {
        uint8_t reg;
        uint16_t value;
    } cases[] = {
        {BQ25620_REG_CHG_CTRL_1, 44U},
        {BQ25620_REG_CHG_CTRL_3, 480U},
        {BQ25620_REG_CHG_CTRL_4, 160U},
    };

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        xy_bq25620_t dev;
        xy_charger_device_status_t status;

        reset_fake_i2c();
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                              xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
        g_regs[BQ25620_REG_CHG_STAT_0] = BQ25620_STAT_CHG_FAST | 0x03U;
        set_reg_u16(BQ25620_REG_CHG_CTRL_1, (160U / BQ25620_ICHG_STEP_mA) << 6U);
        set_reg_u16(BQ25620_REG_CHG_CTRL_3, (4200U / BQ25620_VREG_STEP_mV) << 3U);
        set_reg_u16(BQ25620_REG_CHG_CTRL_4, (500U / BQ25620_ILIM_STEP_mA) << 4U);
        if (cases[index].reg == BQ25620_REG_CHG_CTRL_1) {
            set_reg_u16(cases[index].reg, (uint16_t)cases[index].value << 6U);
        } else if (cases[index].reg == BQ25620_REG_CHG_CTRL_3) {
            set_reg_u16(cases[index].reg, (uint16_t)cases[index].value << 3U);
        } else {
            set_reg_u16(cases[index].reg, (uint16_t)cases[index].value << 4U);
        }
        memset(&status, 0xA5, sizeof(status));

        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_get_status(&dev, &status));
        TEST_ASSERT_EQUAL_UINT32(index == 0U ? BQ25620_ICHG_MAX_mA : 160U,
                                 status.configured_charge_current);
        TEST_ASSERT_EQUAL_UINT32(index == 1U ? BQ25620_VREG_MAX_mV : 4200U,
                                 status.configured_charge_voltage);
        TEST_ASSERT_EQUAL_UINT32(index == 2U ? BQ25620_ILIM_MAX_mA : 500U,
                                 status.configured_input_current_limit);
    }
}

static void test_config_and_range_validation(void)
{
    xy_bq25620_t dev;
    unsigned tx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6B));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_set_charge_current(&dev, BQ25620_ICHG_MIN_mA));
    TEST_ASSERT_EQUAL_HEX16((BQ25620_ICHG_MIN_mA / BQ25620_ICHG_STEP_mA) << 6U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_1));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_current(&dev, 160U));
    TEST_ASSERT_EQUAL_HEX16((160U / BQ25620_ICHG_STEP_mA) << 6U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_1));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_set_charge_current(&dev, BQ25620_ICHG_MAX_mA));
    TEST_ASSERT_EQUAL_HEX16((BQ25620_ICHG_MAX_mA / BQ25620_ICHG_STEP_mA) << 6U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_1));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_set_charge_voltage(&dev, BQ25620_VREG_MIN_mV));
    TEST_ASSERT_EQUAL_HEX16((BQ25620_VREG_MIN_mV / BQ25620_VREG_STEP_mV) << 3U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_3));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_voltage(&dev, 4200U));
    TEST_ASSERT_EQUAL_HEX16((4200U / BQ25620_VREG_STEP_mV) << 3U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_3));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_set_charge_voltage(&dev, BQ25620_VREG_MAX_mV));
    TEST_ASSERT_EQUAL_HEX16((BQ25620_VREG_MAX_mV / BQ25620_VREG_STEP_mV) << 3U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_3));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_set_input_limit(&dev, BQ25620_ILIM_MIN_mA));
    TEST_ASSERT_EQUAL_HEX16((BQ25620_ILIM_MIN_mA / BQ25620_ILIM_STEP_mA) << 4U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_4));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_input_limit(&dev, 500U));
    TEST_ASSERT_EQUAL_HEX16((500U / BQ25620_ILIM_STEP_mA) << 4U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_4));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_set_input_limit(&dev, BQ25620_ILIM_MAX_mA));

#define ASSERT_SETTER_REJECTED(call)                                                        \
    do {                                                                                     \
        tx_before = xy_hal_i2c_master_transmit_fake.call_count;                              \
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, (call));                              \
        TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);        \
    } while (0)

    ASSERT_SETTER_REJECTED(xy_bq25620_set_charge_current(&dev, BQ25620_ICHG_MIN_mA - 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_charge_current(&dev, BQ25620_ICHG_MAX_mA + 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_charge_voltage(&dev, BQ25620_VREG_MIN_mV - 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_charge_voltage(&dev, BQ25620_VREG_MAX_mV + 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_input_limit(&dev, BQ25620_ILIM_MIN_mA - 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_input_limit(&dev, BQ25620_ILIM_MAX_mA + 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_charge_current(&dev, BQ25620_ICHG_MIN_mA + 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_charge_voltage(&dev, BQ25620_VREG_MIN_mV + 1U));
    ASSERT_SETTER_REJECTED(xy_bq25620_set_input_limit(&dev, BQ25620_ILIM_MIN_mA + 1U));

#undef ASSERT_SETTER_REJECTED
}

static void test_start_stop_and_deinit(void)
{
    xy_bq25620_t dev;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6B));

    g_regs[BQ25620_REG_CHG_CTRL_6] = 0x01;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_start_charge(&dev));
    TEST_ASSERT_BITS_HIGH(BQ25620_EN_CHG, g_regs[BQ25620_REG_CHG_CTRL_6]);
    TEST_ASSERT_BITS_HIGH(0x01, g_regs[BQ25620_REG_CHG_CTRL_6]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_stop_charge(&dev));
    TEST_ASSERT_BITS_LOW(BQ25620_EN_CHG, g_regs[BQ25620_REG_CHG_CTRL_6]);
    TEST_ASSERT_BITS_HIGH(0x01, g_regs[BQ25620_REG_CHG_CTRL_6]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.base.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_handle);
}

static void test_lost_transport_and_failed_deinit_are_fail_closed(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;
    xy_charger_device_status_t snapshot;
    unsigned tx_before;
    unsigned rx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6B));
    memset(&status, 0xA5, sizeof(status));
    snapshot = status;
    tx_before = xy_hal_i2c_master_transmit_fake.call_count;
    rx_before = xy_hal_i2c_master_receive_fake.call_count;
    dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_start_charge(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_deinit(&dev));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &status, sizeof(status));
    TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(rx_before, xy_hal_i2c_master_receive_fake.call_count);
}

static void test_deinit_transport_failures_preserve_live_owner_for_retry(void)
{
    static const struct {
        bool fail_receive;
        xy_hal_error_t error;
        int expected;
    } cases[] = {
        {true, XY_HAL_ERROR_TIMEOUT, XY_DEVICE_TIMEOUT},
        {false, XY_HAL_ERROR_IO, XY_DEVICE_IO_ERROR},
    };

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        xy_bq25620_t dev;
        xy_bq25620_t snapshot;
        unsigned tx_before;
        unsigned rx_before;

        reset_fake_i2c();
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                              xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
        g_regs[BQ25620_REG_CHG_CTRL_6] = BQ25620_EN_CHG | 0x01U;
        snapshot = dev;
        tx_before = xy_hal_i2c_master_transmit_fake.call_count;
        rx_before = xy_hal_i2c_master_receive_fake.call_count;
        g_injected_error = cases[index].error;
        if (cases[index].fail_receive) {
            g_fail_rx_call = rx_before + 1U;
        } else {
            g_fail_tx_call = tx_before + 2U;
        }

        TEST_ASSERT_EQUAL_INT(cases[index].expected, xy_bq25620_deinit(&dev));
        TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(snapshot));
        TEST_ASSERT_EQUAL_HEX8(BQ25620_EN_CHG | 0x01U,
                               g_regs[BQ25620_REG_CHG_CTRL_6]);
        TEST_ASSERT_EQUAL_UINT8(1U, dev.base.base.initialized);
        TEST_ASSERT_EQUAL_PTR(g_expected_i2c, dev.i2c_handle);

        g_fail_rx_call = 0U;
        g_fail_tx_call = 0U;
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_deinit(&dev));
        TEST_ASSERT_EQUAL_MEMORY(&(xy_bq25620_t){0}, &dev, sizeof(dev));
        TEST_ASSERT_BITS_LOW(BQ25620_EN_CHG, g_regs[BQ25620_REG_CHG_CTRL_6]);
        TEST_ASSERT_BITS_HIGH(0x01U, g_regs[BQ25620_REG_CHG_CTRL_6]);
    }
}

static void test_init_failure_preserves_caller_storage(void)
{
    xy_bq25620_t dev;
    xy_bq25620_t snapshot;

    memset(&dev, 0xA5, sizeof(dev));
    snapshot = dev;
    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bq25620_init(&dev, g_expected_i2c, 0x6AU));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT(0U, xy_hal_i2c_master_transmit_fake.call_count);

    memset(&dev, 0xA5, sizeof(dev));
    snapshot = dev;
    reset_fake_i2c();
    g_regs[BQ25620_REG_DEVICE_ID] = BQ25622_PART_NUMBER | 0x02U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_SUPPORT,
                          xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));

    memset(&dev, 0x5A, sizeof(dev));
    snapshot = dev;
    reset_fake_i2c();
    g_fail_rx_call = xy_hal_i2c_master_receive_fake.call_count + 1U;
    g_injected_error = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));
}

static void test_failed_reinit_preserves_live_owner(void)
{
    xy_bq25620_t dev;
    xy_bq25620_t snapshot;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    snapshot = dev;
    g_fail_rx_call = xy_hal_i2c_master_receive_fake.call_count + 1U;
    g_injected_error = XY_HAL_ERROR_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(snapshot));
    TEST_ASSERT_EQUAL_PTR(g_expected_i2c, dev.i2c_handle);
}

static void test_full_config_stops_at_first_write_error(void)
{
    static const uint8_t registers[] = {
        BQ25620_REG_CHG_CTRL_1,
        BQ25620_REG_CHG_CTRL_3,
        BQ25620_REG_CHG_CTRL_4,
        BQ25620_REG_CHG_CTRL_2,
        BQ25620_REG_CHG_CTRL_5,
        BQ25620_REG_CHG_CTRL_0,
    };
    static const unsigned reads_before_write[] = {1U, 2U, 3U, 4U, 5U, 6U};
    const xy_charger_device_config_t config = {
        .input_current_limit = 500U,
        .charge_current = 160U,
        .charge_voltage = 4200U,
        .precharge_current = 100U,
        .termination_current = 60U,
        .recharge_threshold = 200U,
        .auto_recharge = true,
    };

    for (unsigned failed_write = 0U; failed_write < sizeof(registers); ++failed_write) {
        xy_bq25620_t dev;
        unsigned tx_before;

        reset_fake_i2c();
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                              xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
        tx_before = xy_hal_i2c_master_transmit_fake.call_count;
        g_fail_tx_call = tx_before + failed_write + reads_before_write[failed_write] + 1U;

        TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR,
                              xy_bq25620_configure(&dev, &config));
        TEST_ASSERT_EQUAL_UINT(g_fail_tx_call,
                               xy_hal_i2c_master_transmit_fake.call_count);
        for (unsigned later = failed_write; later < sizeof(registers); ++later) {
            TEST_ASSERT_EQUAL_HEX8(0U, g_regs[registers[later]]);
        }
        TEST_ASSERT_EQUAL_UINT8(1U, dev.base.base.initialized);
    }
}

static void test_full_config_requires_live_owner(void)
{
    xy_bq25620_t dev;
    const xy_charger_device_config_t config = {0};
    unsigned tx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    tx_before = xy_hal_i2c_master_transmit_fake.call_count;
    dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bq25620_configure(&dev, &config));
    TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);
}

static void test_full_config_public_api_rejects_null_without_io(void)
{
    xy_bq25620_t dev;
    const xy_charger_device_config_t config = {0};
    unsigned tx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    tx_before = xy_hal_i2c_master_transmit_fake.call_count;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_configure(NULL, &config));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_configure(&dev, NULL));
    TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);
}

static void test_full_config_rejects_out_of_range_values_without_io(void)
{
    xy_bq25620_t dev;
    xy_charger_device_config_t config = {
        .input_current_limit = 500U,
        .charge_current = 800U,
        .charge_voltage = 4200U,
        .precharge_current = 100U,
        .termination_current = 60U,
        .recharge_threshold = 100U,
        .auto_recharge = true,
    };
    unsigned tx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));

#define ASSERT_CONFIG_REJECTED(field, value)                                                \
    do {                                                                                     \
        xy_charger_device_config_t invalid = config;                                         \
        invalid.field = (value);                                                             \
        tx_before = xy_hal_i2c_master_transmit_fake.call_count;                              \
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,                                       \
                              xy_bq25620_configure(&dev, &invalid));                         \
        TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);        \
    } while (0)

    ASSERT_CONFIG_REJECTED(input_current_limit, 99U);
    ASSERT_CONFIG_REJECTED(input_current_limit, 3201U);
    ASSERT_CONFIG_REJECTED(charge_current, 79U);
    ASSERT_CONFIG_REJECTED(charge_current, 3521U);
    ASSERT_CONFIG_REJECTED(charge_voltage, 3499U);
    ASSERT_CONFIG_REJECTED(charge_voltage, 4801U);
    ASSERT_CONFIG_REJECTED(precharge_current, 19U);
    ASSERT_CONFIG_REJECTED(precharge_current, 621U);
    ASSERT_CONFIG_REJECTED(termination_current, 9U);
    ASSERT_CONFIG_REJECTED(termination_current, 621U);
    ASSERT_CONFIG_REJECTED(recharge_threshold, 99U);
    ASSERT_CONFIG_REJECTED(recharge_threshold, 300U);
    ASSERT_CONFIG_REJECTED(input_current_limit, 110U);
    ASSERT_CONFIG_REJECTED(charge_current, 81U);
    ASSERT_CONFIG_REJECTED(charge_voltage, 3501U);
    ASSERT_CONFIG_REJECTED(precharge_current, 21U);
    ASSERT_CONFIG_REJECTED(termination_current, 11U);
    ASSERT_CONFIG_REJECTED(recharge_threshold, 150U);

#undef ASSERT_CONFIG_REJECTED
}

static void test_lost_outer_lifecycle_blocks_public_and_callback_paths(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;
    xy_charger_device_status_t snapshot;
    uint8_t value = 0xA5U;
    unsigned tx_before;
    unsigned rx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    memset(&status, 0xA5, sizeof(status));
    snapshot = status;
    tx_before = xy_hal_i2c_master_transmit_fake.call_count;
    rx_before = xy_hal_i2c_master_receive_fake.call_count;
    dev.base.base.initialized = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, &value));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_start_charge(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bq25620_deinit(&dev));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &status, sizeof(status));
    TEST_ASSERT_EQUAL_HEX8(0xA5U, value);
    TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(rx_before, xy_hal_i2c_master_receive_fake.call_count);
}

static void test_transport_errors_propagate_and_preserve_outputs(void)
{
    xy_bq25620_t dev;
    xy_charger_device_status_t status;
    xy_charger_device_status_t snapshot;
    uint8_t value = 0xA5U;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));

    g_injected_error = XY_HAL_ERROR_TIMEOUT;
    g_fail_tx_call = xy_hal_i2c_master_transmit_fake.call_count + 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, &value));
    TEST_ASSERT_EQUAL_HEX8(0xA5U, value);

    g_fail_tx_call = 0U;
    g_fail_rx_call = xy_hal_i2c_master_receive_fake.call_count + 1U;
    g_injected_error = XY_HAL_ERROR_BUSY;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY,
                          xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, &value));
    TEST_ASSERT_EQUAL_HEX8(0xA5U, value);

    memset(&status, 0xA5, sizeof(status));
    snapshot = status;
    g_fail_rx_call = xy_hal_i2c_master_receive_fake.call_count + 3U;
    g_injected_error = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_bq25620_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &status, sizeof(status));

    g_fail_rx_call = 0U;
    g_fail_tx_call = xy_hal_i2c_master_transmit_fake.call_count + 1U;
    g_injected_error = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_bq25620_start_charge(&dev));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.base.base.initialized);
}

static void test_register_access_rejects_undocumented_addresses_without_io(void)
{
    static const uint8_t invalid_registers[] = {
        0x00U,
        0x01U,
        BQ25620_REG_DEVICE_ID + 1U,
    };
    xy_bq25620_t dev;
    uint8_t value = 0xA5U;
    unsigned tx_before;
    unsigned rx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    tx_before = xy_hal_i2c_master_transmit_fake.call_count;
    rx_before = xy_hal_i2c_master_receive_fake.call_count;

    for (size_t index = 0U;
         index < sizeof(invalid_registers) / sizeof(invalid_registers[0]); ++index) {
        TEST_ASSERT_EQUAL_INT(
            XY_DEVICE_INVALID_PARAM,
            xy_bq25620_read_reg(&dev, invalid_registers[index], &value));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, value);
        TEST_ASSERT_EQUAL_UINT(tx_before, xy_hal_i2c_master_transmit_fake.call_count);
        TEST_ASSERT_EQUAL_UINT(rx_before, xy_hal_i2c_master_receive_fake.call_count);
    }
}

static void test_failed_receive_does_not_publish_hal_written_bytes(void)
{
    xy_bq25620_t dev;
    uint8_t value = 0xA5U;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    g_fail_rx_call = xy_hal_i2c_master_receive_fake.call_count + 1U;
    g_injected_error = XY_HAL_ERROR_IO;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_bq25620_read_reg(&dev, BQ25620_REG_DEVICE_ID, &value));
    TEST_ASSERT_EQUAL_HEX8(0xA5U, value);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.base.base.initialized);
}

static void test_setters_preserve_unrelated_register_bits(void)
{
    xy_bq25620_t dev;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));

    set_reg_u16(BQ25620_REG_CHG_CTRL_1, 0x003FU);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_current(&dev, 160U));
    TEST_ASSERT_EQUAL_HEX16(0x00BFU, get_reg_u16(BQ25620_REG_CHG_CTRL_1));

    set_reg_u16(BQ25620_REG_CHG_CTRL_3, 0x0007U);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_charge_voltage(&dev, 4200U));
    TEST_ASSERT_EQUAL_HEX16(((4200U / 10U) << 3U) | 0x0007U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_3));

    set_reg_u16(BQ25620_REG_CHG_CTRL_4, 0x000FU);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_set_input_limit(&dev, 500U));
    TEST_ASSERT_EQUAL_HEX16(((500U / 20U) << 4U) | 0x000FU,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_4));
}

static void test_setter_read_failure_stops_before_write(void)
{
    xy_bq25620_t dev;
    unsigned tx_before;

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    set_reg_u16(BQ25620_REG_CHG_CTRL_1, 0xA5A5U);
    tx_before = xy_hal_i2c_master_transmit_fake.call_count;
    g_fail_rx_call = xy_hal_i2c_master_receive_fake.call_count + 1U;
    g_injected_error = XY_HAL_ERROR_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_bq25620_set_charge_current(&dev, 160U));
    TEST_ASSERT_EQUAL_UINT(tx_before + 1U, xy_hal_i2c_master_transmit_fake.call_count);
    TEST_ASSERT_EQUAL_HEX16(0xA5A5U, get_reg_u16(BQ25620_REG_CHG_CTRL_1));
}

static void test_full_config_preserves_unrelated_register_bits(void)
{
    xy_bq25620_t dev;
    const xy_charger_device_config_t config = {
        .input_current_limit = 500U,
        .charge_current = 160U,
        .charge_voltage = 4200U,
        .precharge_current = 100U,
        .termination_current = 60U,
        .recharge_threshold = 200U,
        .auto_recharge = true,
    };

    reset_fake_i2c();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_init(&dev, g_expected_i2c, 0x6BU));
    set_reg_u16(BQ25620_REG_CHG_CTRL_1, 0x003FU);
    set_reg_u16(BQ25620_REG_CHG_CTRL_3, 0x0007U);
    set_reg_u16(BQ25620_REG_CHG_CTRL_4, 0x000FU);
    set_reg_u16(BQ25620_REG_CHG_CTRL_2, 0x000FU);
    set_reg_u16(BQ25620_REG_CHG_CTRL_5, 0x0007U);
    g_regs[BQ25620_REG_CHG_CTRL_0] = 0xFEU;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bq25620_configure(&dev, &config));
    TEST_ASSERT_EQUAL_HEX16(((160U / 80U) << 6U) | 0x003FU,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_1));
    TEST_ASSERT_EQUAL_HEX16(((4200U / 10U) << 3U) | 0x0007U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_3));
    TEST_ASSERT_EQUAL_HEX16(((500U / 20U) << 4U) | 0x000FU,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_4));
    TEST_ASSERT_EQUAL_HEX16(((100U / 20U) << 4U) | 0x000FU,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_2));
    TEST_ASSERT_EQUAL_HEX16(((60U / 10U) << 3U) | 0x0007U,
                            get_reg_u16(BQ25620_REG_CHG_CTRL_5));
    TEST_ASSERT_EQUAL_HEX8(0xFFU, g_regs[BQ25620_REG_CHG_CTRL_0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_datasheet_register_map_contract);
    RUN_TEST(test_null_param_validation);
    RUN_TEST(test_init_and_register_io);
    RUN_TEST(test_status_decoding);
    RUN_TEST(test_unknown_status_codes_fail_closed);
    RUN_TEST(test_unknown_ts_state_reports_unknown_fault);
    RUN_TEST(test_known_fault_overrides_done_state);
    RUN_TEST(test_status_accepts_datasheet_maximum_setpoint_encodings);
    RUN_TEST(test_config_and_range_validation);
    RUN_TEST(test_start_stop_and_deinit);
    RUN_TEST(test_lost_transport_and_failed_deinit_are_fail_closed);
    RUN_TEST(test_deinit_transport_failures_preserve_live_owner_for_retry);
    RUN_TEST(test_init_failure_preserves_caller_storage);
    RUN_TEST(test_failed_reinit_preserves_live_owner);
    RUN_TEST(test_full_config_stops_at_first_write_error);
    RUN_TEST(test_full_config_requires_live_owner);
    RUN_TEST(test_full_config_public_api_rejects_null_without_io);
    RUN_TEST(test_full_config_rejects_out_of_range_values_without_io);
    RUN_TEST(test_lost_outer_lifecycle_blocks_public_and_callback_paths);
    RUN_TEST(test_transport_errors_propagate_and_preserve_outputs);
    RUN_TEST(test_register_access_rejects_undocumented_addresses_without_io);
    RUN_TEST(test_failed_receive_does_not_publish_hal_written_bytes);
    RUN_TEST(test_setters_preserve_unrelated_register_bits);
    RUN_TEST(test_setter_read_failure_stops_before_write);
    RUN_TEST(test_full_config_preserves_unrelated_register_bits);
    return UNITY_END();
}
