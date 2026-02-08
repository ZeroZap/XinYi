/**
 * @file driver-bq2562x.c
 * @author Xusheng Chen
 * @brief The source file of TI bq2562x(bq25620/bq25622) battery charger driver.
 * @version 0.1
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Philip Morris Products S.A.
 * All rights reserved.
 *
 */
#include "driver-bq2562x.h"

/** Operation timeout time in ms. */
#define OPERATION_TIMEOUT 100

/** Convert two bytes into half word. */
#define MAKE_HALF_WORD(l, m) (((uint16_t)(m << 8)) | ((uint16_t)l))

/**
 * @brief Read the data from the register of bq2562x driver
 *
 * @param charger_instance The charger instance
 * @param reg The register address of charger driver
 * @param buff The buff to store the read data
 * @return int BQ2562X status
 */
static int32_t read_reg(
    drv_charger_instance_t *charger_instance, uint8_t reg, uint8_t *buff,
    uint8_t len)
{
    uint8_t buf[2] = { reg, 0 };
    int32_t rv     = hal_err_none;

    if (charger_instance == NULL || buff == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    if (len < 1 || len > 2) {
        rv = hal_err_invalid_argument;
        return rv;
    }
    charger_instance->p_i2cinstance->i2c_address = BQ2562X_I2C_SLAVE_ADDRESS;
    rv                                           = hal_i2c.masterreceive(
        charger_instance->p_i2cinstance, buf, len, OPERATION_TIMEOUT);
    if (hal_err_none == rv) {
        buff[0] = buf[0];
        if (2 == len) {
            buff[1] = buf[1];
        }
    }
    return rv;
}

/**
 * @brief Write the data to the register of bq2562x driver
 *
 * @param charger_instance The charger instance
 * @param reg The register address of charger driver
 * @param buff The buff to store the read data
 * @return int BQ2562X status
 */
static int32_t write_reg(
    drv_charger_instance_t *charger_instance, uint8_t reg, uint8_t *buff,
    uint8_t len)
{
    int32_t rv     = hal_err_none;
    uint8_t buf[3] = { reg, 0, 0 };

    if (charger_instance == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    if (2 == len) {
        buf[1] = buff[0];
        buf[2] = buff[1];
    } else {
        buf[1] = buff[0];
    }
    charger_instance->p_i2cinstance->i2c_address = BQ2562X_I2C_SLAVE_ADDRESS;
    rv                                           = hal_i2c.mastertransmit(
        charger_instance->p_i2cinstance, buf, len + 1, OPERATION_TIMEOUT);
    return rv;
}

/**
 * @brief Calculate the closest register value.
 *
 * @param charger_instance The charger instance.
 * @param value The data which set to the register.
 * @param min The minimum value that can be set for this data.
 * @param max The maximum value that can be set for this data.
 * @param step The step that can be set for this data.
 *
 * @return uint16_t adjusted closest value.
 */
static uint16_t closest_value(
    drv_charger_instance_t *charger_instance, uint16_t value, uint16_t min,
    uint16_t max, uint16_t step)
{
    if (value > 0 && value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value - (value % step);
}

/**
 * @brief  reset bq2562x chip
 *
 * @param charger_instance The charger instance
 * @return int BQ2562X status
 */
static int32_t register_reset(drv_charger_instance_t *charger_instance)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG17_CHARGER_CONTROL_2, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val |= BQ2562X_REG_RST_RESET;
        rv = write_reg(
            charger_instance, BQ2562X_REG17_CHARGER_CONTROL_2, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief Watchdog reset bq2562x chip
 *
 * @param charger_instance The charger instance
 * @return int BQ2562X status
 */
static int32_t watchdog_reset(drv_charger_instance_t *charger_instance)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG16_CHARGER_CONTROL_1, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val |= BQ2562X_IWDG_RESET;
        rv = write_reg(
            charger_instance, BQ2562X_REG16_CHARGER_CONTROL_1, &reg_val, 1);
    }
    return rv;
}


/**
 * @brief  disable bq2562x iwdg
 *
 * @param charger_instance The charger instance
 * @return int BQ2562X status
 */
static int32_t watchdog_disable(drv_charger_instance_t *charger_instance)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG16_CHARGER_CONTROL_1, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_IWDG_MSK;
        reg_val |= BQ2562X_IWDG_DISABLE << BQ2562X_IWDG_SFT;
        rv = write_reg(
            charger_instance, BQ2562X_REG16_CHARGER_CONTROL_1, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief  Enable or distable adc
 *
 * @param charger_instance The charger instance
 * @return int BQ2562X status
 */
static int32_t
adc_enable(drv_charger_instance_t *charger_instance, uint8_t enable)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(charger_instance, BQ2562X_REG26_ADC_CONTROL, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_ADC_EN_MSK;
        reg_val |= BQ2562X_ADC_EN_ENABLE << BQ2562X_ADC_EN_SFT;
        rv =
            write_reg(charger_instance, BQ2562X_REG26_ADC_CONTROL, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief  disable bq2562x iwdg
 *
 * @param charger_instance The charger instance
 * @return int BQ2562X status
 */
static int32_t auto_dpdm_disable(drv_charger_instance_t *charger_instance)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_AUDO_DPDM_MSK;
        reg_val |= BQ2562X_AUDO_DPDM_DISABLE << BQ2562X_AUTO_DPDM_SFT;
        rv = write_reg(
            charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief Read charger chip id
 *
 * @param charger_instance The charger instance
 * @param id Store the charger chip id
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_device_id(
    drv_charger_instance_t *charger_instance, uint8_t *id)
{

    int32_t rv =
        read_reg(charger_instance, BQ2562X_REG38_PART_INFORMATION, id, 1);

    if (hal_err_none == rv) {
        *id &= BQ2562X_PN_MSK;
    }
    return rv;
}

/**
 * @brief Charger control.
 *
 * @param charger_instance The charger instance.
 * @param enable Enable or disable the charging.
 *
 * @return int BQ2562X status.
 */
int32_t driver_bq2562x_set_control_charge(
    drv_charger_instance_t *charger_instance, uint8_t enable)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG16_CHARGER_CONTROL_1, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_EN_CHG_MSK;
        if (enable) {
            reg_val |= BQ2562X_EN_CHG_ENABLE << BQ2562X_EN_CHG_SFT;
        } else {
            reg_val |= BQ2562X_EN_CHG_DISABLE << BQ2562X_EN_CHG_SFT;
        }
        rv = write_reg(
            charger_instance, BQ2562X_REG16_CHARGER_CONTROL_1, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief Control temperature sensor.
 *
 * @param charger_instance The charger instance.
 * @param enable Enable or disable temperature sensor.
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_set_control_ts(
    drv_charger_instance_t *charger_instance, uint8_t enable)
{
    (void)charger_instance;
    (void)enable;

    return hal_err_not_supported;
}

/**
 * @brief Set charge safety timer.
 *
 * @param charger_instance The charger instance.
 * @param timer One of the @ref bq2562x_charge_timer_t which set to the register
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_set_charge_safety_timer(
    drv_charger_instance_t *charger_instance, uint8_t timer)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_CHG_TMR_SET_MSK;
        reg_val |= (timer & BQ2562X_CHG_TMR_SET_MSK);
        rv = write_reg(
            charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief
 * @param charger_instance The charger instance.
 * @param state The state data which get from the register.
 *
 * @return int BQ2562X status.
 */
int32_t driver_bq2562x_get_charge_state(
    drv_charger_instance_t *charger_instance, uint8_t *state)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    if (state == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv =
        read_reg(charger_instance, BQ2562X_REG1E_CHARGER_STATUS_1, &reg_val, 1);
    if (hal_err_none == rv) {
        *state =
            (reg_val & BQ2562X_REG1E_CHARGER_STATUS_1) >> BQ2562X_CHG_STAT_SFT;
    }
    return rv;
}

/**
 * @brief Enable otg power
 *
 * @param charger_instance The charger instance
 * @param enabled: Enable or Disable which set to the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_set_otg_power(
    drv_charger_instance_t *charger_instance, uint8_t enabled)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG18_CHARGER_CONTROL_3, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_EN_OTG_MSK;
        if (enabled) {
            reg_val |= BQ2562X_EN_OTG_ENABLE << BQ2562X_EN_OTG_SFT;
        } else {
            reg_val |= BQ2562X_EN_OTG_DISABLE << BQ2562X_EN_OTG_SFT;
        }
        rv = write_reg(
            charger_instance, BQ2562X_REG18_CHARGER_CONTROL_3, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief Set the input current
 *
 * @param charger_instance The charger instance
 * @param input_current The input current data which set to the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_set_input_current(
    drv_charger_instance_t *charger_instance, uint16_t input_current)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;
    uint16_t val     = 0;

    rv = read_reg(charger_instance, BQ2562X_REG06_INPUT_CURRENT_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        val = closest_value(
            charger_instance, input_current, BQ2562X_IINDPM_MIN,
            BQ2562X_IINDPM_MAX, BQ2562X_IINDPM_STEP);

        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);
        reg_val &= ~BQ2562X_IINDPM_MSK;
        reg_val |= (val / BQ2562X_IINDPM_STEP) << BQ2562X_IINDPM_SFT;
        buf[0] = reg_val & 0xff;
        buf[1] = (reg_val >> 8) & 0xff;
        rv     = write_reg(
            charger_instance, BQ2562X_REG06_INPUT_CURRENT_LIMIT, buf, 2);
    }
    return rv;
}

/**
 * @brief Get the input current
 *
 * @param charger_instance The charger instance
 * @param input_current The input current data which get from the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_input_current(
    drv_charger_instance_t *charger_instance, uint16_t *input_current)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (input_current == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG06_INPUT_CURRENT_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *input_current = ((reg_val & BQ2562X_IINDPM_MSK) >> BQ2562X_IINDPM_SFT)
            * BQ2562X_IINDPM_STEP;
    }
    return rv;
}

/**
 * @brief Get the target current
 *
 * @param charger_instance The charger instance
 * @param current The target current data which get from the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_current(
    drv_charger_instance_t *charger_instance, uint16_t *current)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (current == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG02_CHARGE_CURRENT_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *current = ((reg_val & BQ2562X_ICHG_MSK) >> BQ2562X_ICHG_SFT)
            * BQ2562X_ICHG_STEP;
    }
    return rv;
}

/**
 * @brief Set the target current
 *
 * @param charger_instance The charger instance
 * @param current The target current data which set to the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_set_current(
    drv_charger_instance_t *charger_instance, uint16_t current)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;
    uint16_t val     = 0;

    rv = read_reg(charger_instance, BQ2562X_REG02_CHARGE_CURRENT_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        val = closest_value(
            charger_instance, current, BQ2562X_ICHG_MIN, BQ2562X_ICHG_MAX,
            BQ2562X_ICHG_STEP);
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);
        reg_val &= ~BQ2562X_ICHG_MSK;
        reg_val |= (val / BQ2562X_ICHG_STEP) << BQ2562X_ICHG_SFT;
        buf[0] = reg_val & 0xff;
        buf[1] = (reg_val >> 8) & 0xff;
        rv     = write_reg(
            charger_instance, BQ2562X_REG02_CHARGE_CURRENT_LIMIT, buf, 2);
    }
    return rv;
}

/**
 * @brief Get the target voltage
 *
 * @param charger_instance The charger instance
 * @param voltage The target voltage data which get from the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_voltage(
    drv_charger_instance_t *charger_instance, uint16_t *voltage)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (voltage == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG04_CHARGE_VOLTAGE_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *voltage = ((reg_val & BQ2562X_VREG_MSK) >> BQ2562X_VREG_SFT)
            * BQ2562X_VREG_STEP;
    }
    return rv;
}

/**
 * @brief Set the target voltage
 *
 * @param charger_instance The charger instance
 * @param voltage The target voltage data which set to the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_set_voltage(
    drv_charger_instance_t *charger_instance, uint16_t voltage)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;
    uint16_t val     = 0;

    rv = read_reg(charger_instance, BQ2562X_REG04_CHARGE_VOLTAGE_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        val = closest_value(
            charger_instance, voltage, BQ2562X_VREG_MIN, BQ2562X_VREG_MAX,
            BQ2562X_VREG_STEP);

        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);
        reg_val &= ~BQ2562X_VREG_MSK;
        reg_val |= (val / BQ2562X_VREG_STEP) << BQ2562X_VREG_SFT;
        buf[0] = reg_val & 0xff;
        buf[1] = (reg_val >> 8) & 0xff;
        rv     = write_reg(
            charger_instance, BQ2562X_REG04_CHARGE_VOLTAGE_LIMIT, buf, 2);
    }
    return rv;
}

/**
 * @brief Get the input voltage
 *
 * @param charger_instance The charger instance
 * @param voltage The input voltage data which get from the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_input_voltage(
    drv_charger_instance_t *charger_instance, uint16_t *voltage)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (voltage == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG08_INPUT_VOLTAGE_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *voltage = ((reg_val & BQ2562X_VINDPM_MSK) >> BQ2562X_VINDPM_SFT)
            * BQ2562X_VINDPM_STEP;
    }
    return rv;
}

/**
 * @brief Set the input voltage limit.
 *
 * @param charger_instance The charger instance.
 * @param voltage The input voltage data which set to the register.
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_set_input_voltage(
    drv_charger_instance_t *charger_instance, uint16_t input_voltage)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;
    uint16_t val     = 0;

    rv = read_reg(charger_instance, BQ2562X_REG08_INPUT_VOLTAGE_LIMIT, buf, 2);
    if (hal_err_none == rv) {
        val = closest_value(
            charger_instance, input_voltage, BQ2562X_VINDPM_MIN,
            BQ2562X_VINDPM_MAX, BQ2562X_VINDPM_STEP);

        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);
        reg_val &= ~BQ2562X_VINDPM_MSK;
        reg_val |= (val / BQ2562X_VINDPM_STEP) << BQ2562X_VINDPM_SFT;
        buf[0] = reg_val & 0xff;
        buf[1] = (reg_val >> 8) & 0xff;
        rv     = write_reg(
            charger_instance, BQ2562X_REG08_INPUT_VOLTAGE_LIMIT, buf, 2);
    }
    return rv;
}

/**
 * @brief Set the termination current in the register of bq2562x.
 *
 * @param charger_instance The charger instance.
 * @param current The current data which set to the register.
 *
 * @return int BQ2562X status.
 */
int32_t driver_bq2562x_set_termination_current(
    drv_charger_instance_t *charger_instance, uint16_t current)
{
    int32_t rv       = hal_err_none;
    uint16_t val     = 0;
    uint16_t reg_val = 0;
    uint8_t buf[2]   = { 0 };

    rv = read_reg(charger_instance, BQ2562X_REG12_TERMINATION_CONTROL, buf, 2);
    if (hal_err_none == rv) {
        val = closest_value(
            charger_instance, current, BQ2562X_ITERM_MIN, BQ2562X_ITERM_MAX,
            BQ2562X_ITERM_STEP);

        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);
        reg_val &= ~BQ2562X_ITERM_MSK;
        reg_val |= (val / BQ2562X_ITERM_STEP) << BQ2562X_ITERM_SFT;
        buf[0] = reg_val & 0xff;
        buf[1] = (reg_val >> 8) & 0xff;
        rv     = write_reg(
            charger_instance, BQ2562X_REG12_TERMINATION_CONTROL, buf, 2);
    }
    return rv;
}

/**
 * @brief Set the pre-charge current in the register of BQ2562X.
 *
 * @param charger_instance The charger instance.
 * @param current The current data which set to the register.
 *
 * @return int32_t BQ2562X status.
 */
int32_t driver_bq2562x_set_pre_charge_current(
    drv_charger_instance_t *charger_instance, uint16_t current)
{
    int32_t rv       = hal_err_none;
    uint16_t val     = 0;
    uint16_t reg_val = 0;
    uint8_t buf[2]   = { 0 };

    rv = read_reg(charger_instance, BQ2562X_REG10_PRECHARGE_CONTROL, buf, 2);
    if (hal_err_none == rv) {
        val = closest_value(
            charger_instance, current, BQ2562X_IPRECHG_MIN, BQ2562X_IPRECHG_MAX,
            BQ2562X_IPRECHG_STEP);

        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);
        reg_val &= ~BQ2562X_IPRECHG_MSK;
        reg_val |= (val / BQ2562X_IPRECHG_STEP) << BQ2562X_IPRECHG_SFT;
        buf[0] = reg_val & 0xff;
        buf[1] = (reg_val >> 8) & 0xff;
        rv     = write_reg(
            charger_instance, BQ2562X_REG10_PRECHARGE_CONTROL, buf, 2);
    }
    return rv;
}

/**
 * @brief Set BQ2562x sysoff.
 *
 * @param charger_instance The charger instance.
 * @param enable Enable or disable the sysoff.
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_set_sysoff(
    drv_charger_instance_t *charger_instance, uint8_t enable)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG18_CHARGER_CONTROL_3, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_BATFET_CTRL_MSK;
        if (enable) {
            reg_val |= BQ2562X_BATFET_CTRL_SHIP << BQ2562X_BATFET_CTRL_SFT;
        } else {
            reg_val |= BQ2562X_BATFET_CTRL_IDLE << BQ2562X_BATFET_CTRL_SFT;
        }
        rv = write_reg(
            charger_instance, BQ2562X_REG18_CHARGER_CONTROL_3, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief Get BQ2562x faults.
 *
 * @param charger_instance The charger instance.
 * @param faults The faults data which get from the register.
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_get_faults(
    drv_charger_instance_t *charger_instance, uint32_t *faults)
{
    int32_t rv          = hal_err_none;
    uint8_t reg_val     = 0;
    uint32_t get_faults = 0;

    if (faults == NULL) {
        rv = hal_err_invalid_pointer;
    } else {
        rv = read_reg(
            charger_instance, BQ2562X_REG1F_FAULT_STATUS_0, &reg_val, 1);
        get_faults = reg_val;
    }
    rv =
        read_reg(charger_instance, BQ2562X_REG1D_CHARGER_STATUS_0, &reg_val, 1);
    if (reg_val & BQ2562X_IWTD_TMR_EXPIRED) {
        get_faults |= BQ2562X_FAULT_WTD_TIMER_EXPIRATION;
    }
    if (reg_val & BQ2562X_CHG_TMR_EXPIRED) {
        get_faults |= BQ2562X_FAULT_CHRG_TIMER_EXPIRATION;
    }
    *faults = get_faults;
    return rv;
}

/**
 * @brief Get battery voltage in mV.
 *
 * @param charger_instance The charger instance.
 * @param voltage The battery voltage data which get from the register
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_get_battery_voltage(
    drv_charger_instance_t *charger_instance, uint16_t *voltage)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (voltage == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG30_VBAT_ADC, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *voltage = ((reg_val & BQ2562X_VBAT_MSK) >> BQ2562X_VBAT_SFT)
            * BQ2562X_VBAT_STEP;
    }
    return rv;
}

/**
 * @brief Get battery current in mA.
 *
 * @param charger_instance The charger instance.
 * @param current The battery current data which get from the register
 *
 * @return int BQ2562x status.
 */
int32_t driver_bq2562x_get_battery_current(
    drv_charger_instance_t *charger_instance, int16_t *current)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (current == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG2A_IBAT_ADC, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *current = ((reg_val & BQ2562X_IBAT_MSK) >> BQ2562X_IBAT_SFT)
            * BQ2562X_IBAT_STEP;
    }
    return rv;
}

/**
 * @brief  Get BUS Input voltage
 *
 * @param charger_instance The charger instance
 * @param current Bus input voltage data which get from the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_bus_voltage(
    drv_charger_instance_t *charger_instance, uint16_t *voltage)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    rv = read_reg(charger_instance, BQ2562X_REG2C_VBUS_ADC, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *voltage = ((reg_val & BQ2562X_VBUS_MSK) >> BQ2562X_VBUS_SFT)
            * BQ2562X_VBUS_STEP;
    }
    return rv;
}

/**
 * @brief  Get BUS Input current
 *
 * @param charger_instance The charger instance
 * @param current Bus input current data which get from the register
 * @return int BQ2562X status
 */
int32_t driver_bq2562x_get_bus_current(
    drv_charger_instance_t *charger_instance, int16_t *current)
{
    int32_t rv       = hal_err_none;
    uint8_t buf[2]   = { 0 };
    uint16_t reg_val = 0;

    if (current == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    rv = read_reg(charger_instance, BQ2562X_REG2A_IBAT_ADC, buf, 2);
    if (hal_err_none == rv) {
        reg_val = MAKE_HALF_WORD(buf[0], buf[1]);

        *current = ((reg_val & BQ2562X_IBUS_MSK) >> BQ2562X_IBUS_SFT)
            * BQ2562X_IBAT_STEP;
    }
    return rv;
}

/**
 * @brief Force DPDM detect
 *
 * @param charger_instance The charger instance.
 * @param enable Enable Or Disable force dpdm detect which set to the register
 *
 * @return int32_t  BQ2562x status.
 */
int32_t driver_bq2562x_set_control_source_detection(
    drv_charger_instance_t *charger_instance, uint8_t enable)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val &= ~BQ2562X_FORCE_DPDM_MSK;
        if (enable) {
            reg_val |= (BQ2562X_FORCE_DPDM_ENABLE << BQ2562X_FORCE_DPDM_SFT);
        } else {
            reg_val |= (BQ2562X_FORCE_DPDM_DISABLE << BQ2562X_FORCE_DPDM_SFT);
        }
        rv = write_reg(
            charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    }
    return rv;
}

/**
 * @brief Get DPDM detection status
 * @param charger_instance The charger instance.
 * @param status DPDM detection status which get from the register
 * @return int32_t  BQ2562x status.
 */
int32_t driver_bq2562x_get_source_detection_status(
    drv_charger_instance_t *charger_instance, uint8_t *status)
{
    int32_t rv      = hal_err_none;
    uint8_t reg_val = 0;

    rv = read_reg(
        charger_instance, BQ2562X_REG15_CHARGE_TIMER_CONTROL, &reg_val, 1);
    if (hal_err_none == rv) {
        if ((reg_val & BQ2562X_DPDM_DONE_MSK) >> BQ2562X_DPDM_DONE_SFT) {
            *status = BQ2562X_DPDM_DETECTING;
        } else {
            *status = BQ2562X_DPDM_DONE;
        }
    }
    return rv;
}

/**
 * @brief Get DPDM detected source type
 * @param charger_instance The charger instance.
 * @param vbus_type DPDM detected source type which get from the register
 * @return int32_t BQ2562x status.
 */
int32_t driver_bq2562x_get_source_type(
    drv_charger_instance_t *charger_instance, uint8_t *vbus_type)
{
    int32_t rv             = hal_err_none;
    uint8_t reg_val        = 0;
    uint16_t input_current = 0;

    rv =
        read_reg(charger_instance, BQ2562X_REG1E_CHARGER_STATUS_1, &reg_val, 1);
    if (hal_err_none == rv) {
        reg_val = (reg_val & BQ2562X_VBUS_TYPE_MSK);
    }
    if (reg_val == BQ2562X_VBUS_TYPE_NSA) {
        rv = driver_bq2562x_get_input_current(charger_instance, &input_current);
        if (hal_err_none == rv) {
            if (input_current == 1000) {
                reg_val = BQ2562X_VBUS_TYPE_NSA_1A;
            } else if (input_current == 2100) {
                reg_val = BQ2562X_VBUS_TYPE_NSA_2P1A;
            } else if (input_current == 2400) {
                reg_val = BQ2562X_VBUS_TYPE_NSA_2P4A;
            } else {
                reg_val = BQ2562X_VBUS_TYPE_UNKNOWN;
            }
        }
    }
    *vbus_type = reg_val;
    return rv;
}

/**
 * @brief bq2562x initialize
 *
 * @param charger_instance The charger instance
 */
int32_t driver_bq2562x_init(drv_charger_instance_t *charger_instance)
{
    hal_errno_t rv = hal_err_none;
    uint8_t val    = 0;

    if (charger_instance == NULL) {
        rv = hal_err_invalid_pointer;
        return rv;
    }
    if (charger_instance->charge_control_pin->gpio_port_x != NULL) {
        if (hal_gpio.initialize(charger_instance->charge_control_pin)) {
            return hal_err_failed;
        }
    }
    if (charger_instance->charge_state_pin->gpio_port_x != NULL) {
        if (hal_gpio.initialize(charger_instance->charge_state_pin)) {
            return hal_err_failed;
        }
    }
    hal_i2c.initialize(charger_instance->p_i2cinstance);
    if (hal_err_none != driver_bq2562x_get_device_id(charger_instance, &val)
        || (val != BQ25620_DEVICE_ID && val != BQ25622_DEVICE_ID)) {
        return hal_err_failed;
    }
    register_reset(charger_instance);
    if (watchdog_reset(charger_instance) != hal_err_none) {
        return hal_err_failed;
    }
    if (watchdog_disable(charger_instance) != hal_err_none) {
        return hal_err_failed;
    }
    if (adc_enable(charger_instance, BQ2562X_ADC_EN_ENABLE)) {
        return hal_err_failed;
    }
    if (auto_dpdm_disable(charger_instance) != hal_err_none) {
        return hal_err_failed;
    }
    return rv;
}

/**
 * @brief List of BQ2562x driver interfaces.
 */
driver_charger_type0_t driver_bq2562x = {
    driver_bq2562x_init,
    driver_bq2562x_get_device_id,
    driver_bq2562x_set_control_ts,
    driver_bq2562x_set_charge_safety_timer,
    driver_bq2562x_set_control_charge,
    driver_bq2562x_get_charge_state,
    driver_bq2562x_set_otg_power,
    driver_bq2562x_set_input_current,
    driver_bq2562x_get_input_current,
    driver_bq2562x_get_current,
    driver_bq2562x_set_current,
    driver_bq2562x_get_voltage,
    driver_bq2562x_set_voltage,
    driver_bq2562x_set_input_voltage,
    driver_bq2562x_set_termination_current,
    driver_bq2562x_set_sysoff,
    driver_bq2562x_get_faults,
    driver_bq2562x_get_battery_voltage,
    driver_bq2562x_get_battery_current,
    driver_bq2562x_set_control_source_detection,
    driver_bq2562x_get_source_detection_status,
    driver_bq2562x_get_source_type,
	};